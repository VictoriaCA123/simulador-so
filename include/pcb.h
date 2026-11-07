/**
 * @file pcb.h
 * @brief Estructuras compartidas del kernel simulado: PCB, colas y mutex.
 *
 * REGLAS DE ORO DE CONCURRENCIA (leer antes de tocar nada):
 *  1. TODO acceso (lectura o escritura) a g_kernel se hace con el mutex tomado.
 *     Forma recomendada:   { KernelLock lock;  ...código... }   // se libera solo
 *  2. Las funciones cuyo nombre termina en "_locked" ASUMEN que quien las llama
 *     ya tiene el mutex. Nunca tomen el lock dentro de una función _locked
 *     (sería un doble lock = deadlock).
 *  3. Las colas guardan PIDs (int), NO punteros. Un PCB* obtenido con
 *     kernel_find_locked() solo es válido mientras se tenga el lock.
 *  4. Nunca hacer sleep/usleep ni esperar input (getline) con el lock tomado.
 *  5. El hilo CPU NO imprime en pantalla; solo el hilo CLI imprime.
 *
 * Responsable: Integrante 3 (Concurrencia, Estado y Métricas).
 */
#ifndef PCB_H
#define PCB_H

#include <pthread.h>

#include <deque>
#include <string>
#include <vector>

/* ------------------------------------------------------------------ */
/* Límites defensivos: cualquier valor fuera de rango se rechaza en CLI */
/* ------------------------------------------------------------------ */
constexpr int MAX_PROCESSES     = 1000;   ///< Tamaño máximo de la tabla
constexpr int MAX_NAME_LEN      = 32;     ///< Largo máximo del nombre
constexpr int MAX_BURST         = 100000; ///< Ráfaga máxima aceptada
constexpr int MIN_PRIORITY      = 1;
constexpr int MAX_PRIORITY      = 5;
constexpr int DEFAULT_PRIORITY  = 3;
constexpr int DEFAULT_QUANTUM   = 2;
constexpr int MAX_QUANTUM       = 1000;
constexpr int MIN_SPEED_MS      = 50;
constexpr int MAX_SPEED_MS      = 5000;
constexpr int DEFAULT_SPEED_MS  = 1000;
constexpr int MAX_ARRIVAL       = 1000000;
constexpr int NO_PID            = -1;     ///< "No hay proceso" / CPU IDLE

/** Estados del ciclo de vida de un proceso. */
enum class ProcState { NEW, READY, RUNNING, BLOCKED, TERMINATED };

/** Políticas de planificación soportadas. */
enum class SchedAlgo { RR, SRTF };

/** Process Control Block. Todos los tiempos se miden en ticks. */
struct PCB {
    int         pid             = 0;
    std::string name;
    ProcState   state           = ProcState::NEW;
    int         priority        = DEFAULT_PRIORITY;

    int arrival_time    = 0;   ///< Tick en que llega al sistema
    int burst_initial   = 0;   ///< Ráfaga total pedida
    int burst_remaining = 0;   ///< Ráfaga que le falta

    int first_run_time  = -1;  ///< Primer tick en CPU (-1 = nunca). RT = first_run_time - arrival_time
    int finish_time     = -1;  ///< Tick en que terminó (-1 = no ha terminado). TAT = finish_time - arrival_time
    int waiting_time    = 0;   ///< Ticks acumulados en READY (WT)
    int quantum_used    = 0;   ///< Ticks consumidos en el quantum actual (RR)
};

/** Un tramo del diagrama de Gantt: [start, end) ejecutó pid (NO_PID = IDLE). */
struct GanttEntry {
    int start;
    int end;
    int pid;
};

/** Estado global compartido entre el hilo CLI y el hilo CPU. */
struct Kernel {
    pthread_mutex_t mutex;        ///< Protege TODOS los campos de abajo
    pthread_cond_t  cond_state;   ///< Despierta al CPU en start / exit

    std::vector<PCB>        table;        ///< Tabla de procesos. índice = pid - 1
    std::deque<int>         ready_queue;  ///< Cola de LISTOS (PIDs)
    int                     running_pid = NO_PID;
    std::vector<GanttEntry> gantt;

    int clock_ticks = 0;   ///< Reloj simulado (NO es el reloj real)
    int busy_ticks  = 0;   ///< Ticks con algún proceso en CPU (para % uso)

    SchedAlgo algo        = SchedAlgo::RR;
    int       quantum     = DEFAULT_QUANTUM;
    int       ms_per_tick = DEFAULT_SPEED_MS;

    bool cpu_running    = false;  ///< true tras "start", false tras "pause"
    bool exit_requested = false;  ///< true tras "exit" o EOF
    int  next_pid       = 1;
};

/** Única instancia global (definida en pcb.cpp). */
extern Kernel g_kernel;

/**
 * Cerrojo RAII: llama a pthread_mutex_lock al crearse y a
 * pthread_mutex_unlock al salir del bloque { }, incluso con return/break.
 * Así es imposible olvidar un unlock.
 */
class KernelLock {
public:
    KernelLock()  { pthread_mutex_lock(&g_kernel.mutex); }
    ~KernelLock() { pthread_mutex_unlock(&g_kernel.mutex); }
    KernelLock(const KernelLock&)            = delete;
    KernelLock& operator=(const KernelLock&) = delete;
};

/* --------------------------- API del kernel --------------------------- */

/** Inicializa mutex, variable de condición y tabla. false si falla. */
bool kernel_init();

/** Destruye mutex y condición. Llamar solo cuando ya no hay hilos vivos. */
void kernel_destroy();

/** Pide a todos los hilos que terminen (toma el lock internamente). Idempotente. */
void kernel_request_shutdown();

/** Texto del estado tal como lo exige el enunciado (READY, RUNNING...). */
const char* state_to_string(ProcState s);

/** "RR" o "SRTF". */
const char* algo_to_string(SchedAlgo a);

/**
 * Crea un PCB nuevo. Si arrival <= clock_ticks pasa directo a READY y entra
 * a la cola; si no, queda en NEW hasta que el CPU lo admita.
 * @return PID asignado, o NO_PID si la tabla está llena.
 * REQUIERE el lock.
 */
int kernel_add_process_locked(const std::string& name, int burst,
                              int priority, int arrival);

/**
 * Busca un proceso por PID. nullptr si no existe.
 * REQUIERE el lock. No guardar el puntero después de soltar el lock.
 */
PCB* kernel_find_locked(int pid);

#endif // PCB_H
