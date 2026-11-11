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

// Límites defensivos del sistema
constexpr int MAX_PROCESSES = 1000;
constexpr int MAX_NAME_LEN  = 32;
constexpr int MAX_BURST     = 100000;
constexpr int MIN_PRIORITY  = 1;

// Estados del proceso según la especificación de la cátedra
enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// Conversión de estado a string para los reportes y la CLI
inline std::string stateToString(ProcessState state) {
    switch (state) {
        case ProcessState::NEW: return "NEW";
        case ProcessState::READY: return "READY";
        case ProcessState::RUNNING: return "RUNNING";
        case ProcessState::BLOCKED: return "BLOCKED";
        case ProcessState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

// Estructura del Process Control Block (PCB)
struct PCB {
    int pid;                  // Identificador único autoincremental
    std::string name;         // Nombre del proceso
    ProcessState state;       // Estado actual
    int burst_time;           // Ráfaga inicial total
    int remaining_time;       // Ráfaga restante
    int priority;             // Prioridad (1 a 5)
    int arrival_time;         // Tiempo de llegada (ticks)
    
    // Métricas de desempeño exigidas
    int waiting_time;         // Tiempo de espera acumulado (WT)
    int turnaround_time;      // Tiempo de retorno (TAT)
    int response_time;        // Tiempo de respuesta (RT)
    bool has_responded;       // Bandera para el primer cálculo de RT
    int completion_time;      // Instante de finalización
};

// Declaración de función auxiliar de creación
PCB createProcess(int pid, const std::string& name, int burst_time, int priority, int arrival_time);

#endif // PCB_H