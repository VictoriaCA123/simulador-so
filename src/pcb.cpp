/**
 * @file pcb.cpp
 * @brief Implementación del estado global del kernel y su API básica.
 * Responsable: Integrante 3.
 */
#include "pcb.h"

Kernel g_kernel;

bool kernel_init() {
    if (pthread_mutex_init(&g_kernel.mutex, nullptr) != 0) return false;
    if (pthread_cond_init(&g_kernel.cond_state, nullptr) != 0) {
        pthread_mutex_destroy(&g_kernel.mutex);
        return false;
    }
    // Reservamos toda la capacidad de una vez: el vector NUNCA se realoja,
    // así un PCB* tomado bajo el lock no queda colgando por un push_back.
    g_kernel.table.reserve(MAX_PROCESSES);
    return true;
}

void kernel_destroy() {
    pthread_cond_destroy(&g_kernel.cond_state);
    pthread_mutex_destroy(&g_kernel.mutex);
}

void kernel_request_shutdown() {
    KernelLock lock;
    g_kernel.exit_requested = true;
    g_kernel.cpu_running    = false;
    pthread_cond_broadcast(&g_kernel.cond_state);  // despierta al CPU si estaba en pausa
}

const char* state_to_string(ProcState s) {
    switch (s) {
        case ProcState::NEW:        return "NEW";
        case ProcState::READY:      return "READY";
        case ProcState::RUNNING:    return "RUNNING";
        case ProcState::BLOCKED:    return "BLOCKED";
        case ProcState::TERMINATED: return "TERMINATED";
    }
    return "UNKNOWN";
}

const char* algo_to_string(SchedAlgo a) {
    return a == SchedAlgo::RR ? "RR" : "SRTF";
}

int kernel_add_process_locked(const std::string& name, int burst,
                              int priority, int arrival) {
    if (static_cast<int>(g_kernel.table.size()) >= MAX_PROCESSES) return NO_PID;

    PCB p;
    p.pid             = g_kernel.next_pid++;
    p.name            = name;
    p.priority        = priority;
    p.arrival_time    = arrival;
    p.burst_initial   = burst;
    p.burst_remaining = burst;
    p.state           = ProcState::NEW;

    if (arrival <= g_kernel.clock_ticks) {
        p.state = ProcState::READY;
        g_kernel.ready_queue.push_back(p.pid);
    }
    g_kernel.table.push_back(p);
    return p.pid;
}

PCB* kernel_find_locked(int pid) {
    if (pid < 1 || pid > static_cast<int>(g_kernel.table.size())) return nullptr;
    return &g_kernel.table[pid - 1];
}
