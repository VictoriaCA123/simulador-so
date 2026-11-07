/**
 * @file cli.cpp
 * @brief Hilo CLI: lee comandos, valida y despacha.
 * Responsable: Integrante 1.
 *
 * PATRÓN OBLIGATORIO para cada comando (ver cmd_create como modelo):
 *   1. check_argc(...)      -> cantidad de argumentos
 *   2. get_int_arg(...)     -> tipo y rango de cada número
 *   3. { KernelLock lock; ... }  -> recién ahí tocar g_kernel
 *   4. Imprimir [OK] o [ERROR ...] FUERA del lock si es posible.
 */
#include "cli.h"

#include <sys/stat.h>

#include <cerrno>
#include <climits>
#include <fstream>
#include <iostream>
#include <vector>

#include "pcb.h"
#include "report.h"
#include "utils.h"

constexpr size_t MAX_LINE_LEN = 512;

/* ======================= Mensajes de error estándar ======================= */

static void err_syntax(const std::string& msg, const char* usage) {
    std::cout << "[ERROR DE SINTAXIS]: " << msg << "\nUso correcto: " << usage << "\n";
}
static void err_type(const std::string& msg)  { std::cout << "[ERROR DE TIPO]: " << msg << "\n"; }
static void err_param(const std::string& msg) { std::cout << "[ERROR DE PARAMETRO]: " << msg << "\n"; }

/** Valida que args.size() esté en [min_argc, max_argc] (incluye el nombre del comando). */
static bool check_argc(const Args& a, size_t min_argc, size_t max_argc, const char* usage) {
    if (a.size() < min_argc) { err_syntax("Parámetros insuficientes.", usage); return false; }
    if (a.size() > max_argc) { err_syntax("Demasiados parámetros.", usage);   return false; }
    return true;
}

/** Convierte un argumento a entero en [min,max]; imprime el error adecuado si falla. */
static bool get_int_arg(const std::string& s, int min, int max, const std::string& what, int& out) {
    switch (parse_int(s, min, max, out)) {
        case ParseStatus::OK:
            return true;
        case ParseStatus::NOT_INTEGER:
            err_type(what + " debe ser un número entero (recibido: '" + s + "').");
            return false;
        case ParseStatus::OUT_OF_RANGE:
            err_param(what + " debe estar entre " + std::to_string(min) + " y " +
                      std::to_string(max) + ".");
            return false;
    }
    return false;
}

/* ================================ Comandos ================================ */

static bool g_cli_exit = false;

/** MODELO: copiar este patrón para los demás comandos. */
static void cmd_create(const Args& a) {
    const char* usage = "create <nombre> <burst_time> [prioridad]";
    if (!check_argc(a, 3, 4, usage)) return;

    if (!is_valid_name(a[1])) {
        err_param("Nombre inválido. Use 1-" + std::to_string(MAX_NAME_LEN) +
                  " caracteres: letras, dígitos, '_', '-' o '.'.");
        return;
    }
    int burst = 0;
    if (!get_int_arg(a[2], 1, MAX_BURST, "El burst", burst)) return;

    int prio = DEFAULT_PRIORITY;
    if (a.size() == 4 && !get_int_arg(a[3], MIN_PRIORITY, MAX_PRIORITY, "La prioridad", prio)) return;

    int pid;
    {
        KernelLock lock;
        pid = kernel_add_process_locked(a[1], burst, prio, g_kernel.clock_ticks);
    }
    if (pid == NO_PID) {
        err_param("Tabla de procesos llena (máximo " + std::to_string(MAX_PROCESSES) + ").");
        return;
    }
    std::cout << "[OK]: Proceso '" << a[1] << "' creado con PID " << pid << ".\n";
}

static void cmd_load(const Args& a) {
    if (!check_argc(a, 2, 2, "load <archivo>")) return;
    int n = cli_load_workload(a[1]);   // imprime sus propios errores
    if (n > 0) {
        std::cout << "[OK]: " << n << " proceso(s) cargado(s) desde '" << a[1] << "'.\n";
    }
}

static void cmd_ps(const Args& a) {
    if (!check_argc(a, 1, 1, "ps")) return;
    report_ps();
}

static void cmd_pcb(const Args& a) {
    if (!check_argc(a, 2, 2, "pcb <PID>")) return;

    int pid = 0;
    ParseStatus st = parse_int(a[1], INT_MIN, INT_MAX, pid);
    bool es_negativo = !a[1].empty() && a[1][0] == '-';

    // Caso 1: no es un entero, o es <= 0  
    if (st == ParseStatus::NOT_INTEGER ||
        (st == ParseStatus::OK && pid <= 0) ||
        (st == ParseStatus::OUT_OF_RANGE && es_negativo)) {
        err_type("El PID debe ser un número entero positivo.");
        return;
    }
    // Caso 2: es un entero positivo, pero no existe (o es tan grande que no cabe en int)
    if (st == ParseStatus::OUT_OF_RANGE || !report_pcb(pid)) {
        err_param("No existe un proceso con PID " + a[1] + ".");
    }
}

static void cmd_set_sched(const Args& a) {
    const char* usage = "set_sched <RR|SRTF> [quantum]";
    if (!check_argc(a, 2, 3, usage)) return;

    SchedAlgo algo;
    if (a[1] == "RR") {
        algo = SchedAlgo::RR;
    } else if (a[1] == "SRTF") {
        algo = SchedAlgo::SRTF;
        if (a.size() == 3) { err_param("SRTF no usa quantum."); return; }
    } else {
        err_param("Algoritmo no reconocido. Algoritmos válidos: RR, SRTF.");
        return;
    }

    int quantum = DEFAULT_QUANTUM;
    if (a.size() == 3 && !get_int_arg(a[2], 1, MAX_QUANTUM, "El quantum", quantum)) return;

    {
        KernelLock lock;
        g_kernel.algo = algo;
        if (algo == SchedAlgo::RR) g_kernel.quantum = quantum;
    }
    std::cout << "[OK]: Planificador = " << a[1];
    if (algo == SchedAlgo::RR) std::cout << " (quantum = " << quantum << ")";
    std::cout << ".\n";
}

static void cmd_set_speed(const Args& a) {
    if (!check_argc(a, 2, 2, "set_speed <ms_por_tick>")) return;
    int ms = 0;
    if (!get_int_arg(a[1], MIN_SPEED_MS, MAX_SPEED_MS, "La velocidad (ms)", ms)) return;
    {
        KernelLock lock;
        g_kernel.ms_per_tick = ms;
    }
    std::cout << "[OK]: Velocidad = " << ms << " ms por tick.\n";
}

static void cmd_start(const Args& a) {
    if (!check_argc(a, 1, 1, "start")) return;
    bool already;
    {
        KernelLock lock;
        already = g_kernel.cpu_running;
        g_kernel.cpu_running = true;
        pthread_cond_signal(&g_kernel.cond_state);   // despierta al hilo CPU
    }
    std::cout << (already ? "[INFO]: La CPU ya estaba en ejecución.\n"
                          : "[OK]: CPU iniciada.\n");
}

static void cmd_pause(const Args& a) {
    if (!check_argc(a, 1, 1, "pause")) return;
    bool was_running;
    {
        KernelLock lock;
        was_running = g_kernel.cpu_running;
        g_kernel.cpu_running = false;
    }
    std::cout << (was_running ? "[OK]: CPU en pausa.\n"
                              : "[INFO]: La CPU ya estaba en pausa.\n");
}

static void cmd_gantt(const Args& a) {
    if (!check_argc(a, 1, 1, "gantt")) return;
    report_gantt();
}

static void cmd_stats(const Args& a) {
    if (!check_argc(a, 1, 1, "stats")) return;
    report_stats();
}

static void cmd_help(const Args& a) {
    if (!check_argc(a, 1, 1, "help")) return;
    std::cout <<
        "Comandos disponibles:\n"
        "  create <nombre> <burst> [prioridad 1-5]  Crea un proceso\n"
        "  load <archivo>                           Carga procesos desde una traza\n"
        "  ps                                       Tabla de procesos\n"
        "  pcb <PID>                                Detalle del PCB\n"
        "  set_sched <RR|SRTF> [quantum]            Cambia el planificador\n"
        "  set_speed <ms>                           Milisegundos por tick (50-5000)\n"
        "  start | pause                            Inicia / pausa la CPU\n"
        "  gantt | stats                            Diagrama de Gantt / métricas\n"
        "  help | exit                              Ayuda / salir\n";
}

static void cmd_exit(const Args& a) {
    if (!check_argc(a, 1, 1, "exit")) return;
    g_cli_exit = true;
}

/* ============================ Tabla de despacho ============================ */

struct Command {
    const char* name;
    void (*handler)(const Args&);
};

static const Command COMMANDS[] = {
    {"create", cmd_create}, {"load", cmd_load},           {"ps", cmd_ps},
    {"pcb", cmd_pcb},       {"set_sched", cmd_set_sched}, {"set_speed", cmd_set_speed},
    {"start", cmd_start},   {"pause", cmd_pause},         {"gantt", cmd_gantt},
    {"stats", cmd_stats},   {"help", cmd_help},           {"exit", cmd_exit},
};

bool cli_execute_line(const std::string& line) {
    if (line.size() > MAX_LINE_LEN) {
        std::cout << "[ERROR]: Línea demasiado larga (máximo " << MAX_LINE_LEN << " caracteres).\n";
        return true;
    }
    Args args = tokenize(line);
    if (args.empty()) return true;   // Enter vacío: no hacer nada

    for (const Command& c : COMMANDS) {
        if (args[0] == c.name) {
            c.handler(args);
            return !g_cli_exit;
        }
    }
    std::cout << "[ERROR]: Comando '" << args[0] << "' no reconocido. Escriba 'help'.\n";
    return true;
}

/* ============================== Comando load ============================== */

constexpr int MAX_LOAD_ERRORS_SHOWN = 10;   // evita inundar la pantalla con un archivo basura

/** Separa una línea por comas y aplica trim() a cada campo.
 *  Siempre devuelve (número de comas + 1) campos, incluso si alguno está vacío. */
static Args split_csv(const std::string& line) {
    Args fields;
    size_t start = 0;
    while (true) {
        size_t comma = line.find(',', start);
        if (comma == std::string::npos) {            // ya no hay más comas
            fields.push_back(trim(line.substr(start)));
            break;
        }
        fields.push_back(trim(line.substr(start, comma - start)));
        start = comma + 1;                           // seguir después de la coma
    }
    return fields;
}

/** Un proceso leído y validado del archivo, todavía NO insertado en el kernel. */
struct LoadedJob {
    std::string name;
    int burst   = 0;
    int prio    = 0;
    int arrival = 0;
};

/** Valida un campo numérico. Si falla, deja la explicación en `err`. */
static bool load_int_field(const std::string& s, int min, int max, const char* what,
                           int& out, std::string& err) {
    ParseStatus st = parse_int(s, min, max, out);
    if (st == ParseStatus::OK) return true;

    err = std::string(what) + " '" + s + "' ";
    if (st == ParseStatus::NOT_INTEGER) {
        err += "no es un numero entero";
    } else {
        err += "fuera de rango (" + std::to_string(min) + " a " + std::to_string(max) + ")";
    }
    return false;
}

/** Revisa UNA línea (ya recortada). true = válida y `job` queda lleno; false = `err` explica por qué. */
static bool parse_load_line(const std::string& line, LoadedJob& job, std::string& err) {
    if (line.size() > MAX_LINE_LEN) {
        err = "linea demasiado larga";
        return false;
    }
    Args f = split_csv(line);
    if (f.size() != 4) {
        err = "tiene " + std::to_string(f.size()) + " campo(s), se esperaban 4";
        return false;
    }
    if (!is_valid_name(f[0])) {
        err = "nombre invalido (use 1-" + std::to_string(MAX_NAME_LEN) +
              " letras, digitos, '_', '-' o '.')";
        return false;
    }
    job.name = f[0];
    return load_int_field(f[1], 1, MAX_BURST, "burst", job.burst, err) &&
           load_int_field(f[2], MIN_PRIORITY, MAX_PRIORITY, "prioridad", job.prio, err) &&
           load_int_field(f[3], 0, MAX_ARRIVAL, "llegada", job.arrival, err);
}

/** Comprueba que la ruta exista y sea un archivo normal (no una carpeta ni un dispositivo). */
static bool check_regular_file(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        if (errno == EACCES) {
            std::cout << "[ERROR]: Sin permiso para acceder a '" << path << "'.\n";
        } else {
            std::cout << "[ERROR]: El archivo '" << path << "' no existe.\n";
        }
        return false;
    }
    if (S_ISDIR(info.st_mode)) {
        std::cout << "[ERROR]: '" << path << "' es una carpeta, no un archivo.\n";
        return false;
    }
    if (!S_ISREG(info.st_mode)) {
        std::cout << "[ERROR]: '" << path << "' no es un archivo de texto normal.\n";
        return false;
    }
    return true;
}

int cli_load_workload(const std::string& path) {
    // 1) La ruta debe ser un archivo normal que se pueda abrir
    if (!check_regular_file(path)) return -1;

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "[ERROR]: No se pudo abrir '" << path << "' (permisos de lectura).\n";
        return -1;
    }

    // 2) Leer y validar TODO el archivo sin tocar el kernel (sin lock)
    std::vector<LoadedJob> jobs;
    std::string raw;
    std::string err;
    int line_no = 0;
    int skipped = 0;

    while (std::getline(file, raw)) {
        line_no++;
        std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;   // vacía o comentario

        LoadedJob job;
        if (parse_load_line(line, job, err)) {
            jobs.push_back(job);
        } else {
            skipped++;
            if (skipped <= MAX_LOAD_ERRORS_SHOWN) {
                std::cout << "[ERROR DE FORMATO]: Linea " << line_no << ": " << err
                          << ". Se omite.\n";
            }
        }
    }
    if (skipped > MAX_LOAD_ERRORS_SHOWN) {
        std::cout << "  ... y " << (skipped - MAX_LOAD_ERRORS_SHOWN)
                  << " error(es) mas que no se muestran.\n";
    }

    if (jobs.empty()) {
        std::cout << "[ERROR]: '" << path << "' no contiene procesos validos. No se cargo nada.\n";
        return -1;
    }

    // 3) Insertar todo de una vez bajo el lock (carga atómica: todo o nada)
    std::vector<int> pids;
    {
        KernelLock lock;   // ---- inicio de la sección crítica ----
        int libres = MAX_PROCESSES - static_cast<int>(g_kernel.table.size());
        if (static_cast<int>(jobs.size()) <= libres) {
            for (const LoadedJob& j : jobs) {
                pids.push_back(kernel_add_process_locked(j.name, j.burst, j.prio, j.arrival));
            }
        }
    }                      // ---- fin: aquí se libera el mutex solo ----

    // 4) Imprimir resultados FUERA del lock
    if (pids.empty()) {
        std::cout << "[ERROR]: No hay espacio para " << jobs.size()
                  << " procesos en la tabla. No se cargo ninguno.\n";
        return -1;
    }
    for (size_t k = 0; k < pids.size(); ++k) {
        std::cout << "  PID " << pids[k] << " -> " << jobs[k].name
                  << " (llegada en tick " << jobs[k].arrival << ")\n";
    }
    if (skipped > 0) {
        std::cout << "[AVISO]: " << skipped << " linea(s) omitida(s) por errores.\n";
    }
    return static_cast<int>(pids.size());
}

void* cli_thread_main(void* /*arg*/) {
    std::string line;
    while (true) {
        std::cout << "> " << std::flush;
        if (!std::getline(std::cin, line)) {   // EOF (Ctrl+D) o error de lectura
            std::cout << "\n[INFO]: Fin de entrada. Cerrando simulador...\n";
            break;
        }
        if (!cli_execute_line(line)) break;    // "exit"
    }
    kernel_request_shutdown();
    return nullptr;
}
