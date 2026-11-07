/**
 * @file cpu.cpp
 * @brief Hilo Motor de Scheduling.
 * Responsable: Integrante 2.
 *
 * cpu_thread_main ya está terminado (infraestructura de hilos).
 * Lo que falta programar está marcado con TODO.
 */
#include "cpu.h"

#include <unistd.h>

#include "pcb.h"
#include "report.h"

/** Duerme ms milisegundos en trozos de 10 ms para reaccionar rápido a "exit". */
static void cpu_sleep_interruptible(int ms) {
    const int chunk = 10;
    for (int slept = 0; slept < ms; slept += chunk) {
        {
            KernelLock lock;
            if (g_kernel.exit_requested) return;
        }
        usleep(chunk * 1000);
    }
}

void* cpu_thread_main(void* /*arg*/) {
    while (true) {
        int sleep_ms;
        {
            KernelLock lock;
            // En pausa: dormir sin consumir CPU real hasta "start" o "exit".
            while (!g_kernel.cpu_running && !g_kernel.exit_requested) {
                pthread_cond_wait(&g_kernel.cond_state, &g_kernel.mutex);
            }
            if (g_kernel.exit_requested) break;   // KernelLock libera al salir

            cpu_admit_arrivals_locked();
            cpu_tick_locked();
            g_kernel.clock_ticks++;
            sleep_ms = g_kernel.ms_per_tick;
        }
        cpu_sleep_interruptible(sleep_ms);   // SIEMPRE fuera del lock
    }
    return nullptr;
}

void cpu_admit_arrivals_locked() {
    // TODO (Integrante 2): recorrer g_kernel.table; cada PCB en NEW con
    // arrival_time <= g_kernel.clock_ticks -> state = READY y
    // g_kernel.ready_queue.push_back(pid). Ojo con el orden de llegada.
}

void cpu_tick_locked() {
    // TODO (Integrante 2): según g_kernel.algo
    //  1. Si hay running_pid, decidir si se desapropia:
    //       RR   -> quantum_used == g_kernel.quantum  => vuelve al final de la cola
    //       SRTF -> hay en READY uno con menor burst_remaining => vuelve a la cola
    //  2. Si la CPU está libre, elegir con cpu_pick_next_*_locked() y sacarlo de la cola.
    //  3. Si es su primera vez en CPU: first_run_time = clock_ticks.
    //  4. burst_remaining--, quantum_used++, busy_ticks++.
    //  5. waiting_time++ a todos los que están en READY.
    //  6. Si burst_remaining == 0: TERMINATED, finish_time = clock_ticks + 1, running_pid = NO_PID.
    //  7. gantt_record_locked(pid ejecutado o NO_PID si IDLE).
    gantt_record_locked(NO_PID);  // placeholder: por ahora la CPU siempre está IDLE
}

int cpu_pick_next_rr_locked() {
    // TODO (Integrante 2)
    return NO_PID;
}

int cpu_pick_next_srtf_locked() {
    // TODO (Integrante 2): menor burst_remaining; desempate por arrival_time y luego PID.
    return NO_PID;
}
