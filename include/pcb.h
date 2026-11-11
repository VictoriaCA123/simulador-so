/**
 * @file pcb.h
 * @brief Estructuras compartidas del kernel simulado: PCB, colas y mutex.
 *
 * 
 *
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

    int arrival_time    = 0;  
    int burst_initial   = 0;   
    int burst_remaining = 0;   

    int first_run_time  = -1;  
    int finish_time     = -1;  
    int waiting_time    = 0;   
    int quantum_used    = 0;   
};

/** Un tramo del diagrama de Gantt: [start, end) ejecutó pid (NO_PID = IDLE). */
struct GanttEntry {
    int start;
    int end;
    int pid;
};

/** Estado global compartido entre el hilo CLI y el hilo CPU. */
struct Kernel {
    pthread_mutex_t mutex;        
    pthread_cond_t  cond_state;   
    std::vector<PCB>        table;        
    std::deque<int>         ready_queue;  
    int                     running_pid = NO_PID;
    std::vector<GanttEntry> gantt;

    int clock_ticks = 0;   
    int busy_ticks  = 0;   

    SchedAlgo algo        = SchedAlgo::RR;
    int       quantum     = DEFAULT_QUANTUM;
    int       ms_per_tick = DEFAULT_SPEED_MS;

    bool cpu_running    = false;  
    bool exit_requested = false;  
    int  next_pid       = 1;
};

/** Única instancia global (definida en pcb.cpp). */
extern Kernel g_kernel;


class KernelLock {
public:
    KernelLock()  { pthread_mutex_lock(&g_kernel.mutex); }
    ~KernelLock() { pthread_mutex_unlock(&g_kernel.mutex); }
    KernelLock(const KernelLock&)            = delete;
    KernelLock& operator=(const KernelLock&) = delete;
};



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
 * Crea un PCB nuevo. 
 * @return PID asignado, o NO_PID si la tabla está llena.
 * REQUIERE el lock.
 */
int kernel_add_process_locked(const std::string& name, int burst,
                              int priority, int arrival);

/**
 * Busca un proceso por PID. nullptr si no existe.
 */
PCB* kernel_find_locked(int pid);

#endif // PCB_H
