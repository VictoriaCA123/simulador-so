#include "cpu.h"
#include <algorithm>
#include <iterator>
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

//La entrada del hilo CPU. Es un bucle infinito de ticks simulados hasta que se solicita la salida.
void* cpu_thread_main(void* /*arg*/) {
    while (true) {
        int sleep_ms;
        {
            KernelLock lock;
            // En pausa: dormir sin consumir CPU real hasta "start" o "exit".
            while (!g_kernel.cpu_running && !g_kernel.exit_requested) {
                pthread_cond_wait(&g_kernel.cond_state, &g_kernel.mutex);
            }
            if (g_kernel.exit_requested) break;   

            cpu_admit_arrivals_locked();
            cpu_tick_locked();
            g_kernel.clock_ticks++;
            sleep_ms = g_kernel.ms_per_tick;
        }
        cpu_sleep_interruptible(sleep_ms);   
    }
    return nullptr;
}


// Cambia el estado de los procesos NEW a READY. 
// Insetandolos en una lista de espera (ready_queue) en orden de llegada y PID.
// Se requiere que el mutex del kernel esté bloqueado antes de llamar a esta función.
void cpu_admit_arrivals_locked() {
    std::vector<int> arriving_pids;

    for (const PCB& process : g_kernel.table) {
        if (process.state == ProcState::NEW &&
            process.arrival_time <= g_kernel.clock_ticks) {
            arriving_pids.push_back(process.pid);
        }
    }

    std::sort(arriving_pids.begin(), arriving_pids.end(), [](int lhs, int rhs) {
        const PCB& left = g_kernel.table[lhs - 1];
        const PCB& right = g_kernel.table[rhs - 1];

        if (left.arrival_time != right.arrival_time) {
            return left.arrival_time < right.arrival_time;
        }
        return left.pid < right.pid;
    });

    for (int pid : arriving_pids) {
        PCB* process = kernel_find_locked(pid);
        process->state = ProcState::READY;
        g_kernel.ready_queue.push_back(pid);
    }
}


// Simula un tick de CPU y detiene el proceso en ejecución si es necesario por RR o SRTF.
// Actualiza los tiempos de espera de los procesos en READY y registra el tick en el Gantt.
// Tambien al terminar marca el proceso como TERMINATED y actualiza su tiempo de finalización.
void cpu_tick_locked() {
    PCB* running = kernel_find_locked(g_kernel.running_pid);
    bool preempt = false;

    if (running != nullptr) {
        if (g_kernel.algo == SchedAlgo::RR) {
            preempt = running->quantum_used >= g_kernel.quantum;
        } else {
            for (int pid : g_kernel.ready_queue) {
                PCB* ready = kernel_find_locked(pid);
                if (ready != nullptr &&
                    ready->burst_remaining < running->burst_remaining) {
                    preempt = true;
                    break;
                }
            }
        }

        if (preempt) {
            running->state = ProcState::READY;
            running->quantum_used = 0;
            g_kernel.ready_queue.push_back(running->pid);
            g_kernel.running_pid = NO_PID;
        }
    }

    if (g_kernel.running_pid == NO_PID) {
        g_kernel.running_pid = g_kernel.algo == SchedAlgo::RR
            ? cpu_pick_next_rr_locked()
            : cpu_pick_next_srtf_locked();
    }

    const int executed_pid = g_kernel.running_pid;
    PCB* process = kernel_find_locked(executed_pid);
    if (process == nullptr) {
        for (int pid : g_kernel.ready_queue) {
            PCB* ready = kernel_find_locked(pid);
            if (ready != nullptr && ready->state == ProcState::READY) {
                ready->waiting_time++;
            }
        }
        gantt_record_locked(NO_PID);
        return;
    }

    process->state = ProcState::RUNNING;
    if (process->first_run_time < 0) {
        process->first_run_time = g_kernel.clock_ticks;
    }
    process->burst_remaining--;
    process->quantum_used++;
    g_kernel.busy_ticks++;

    for (int pid : g_kernel.ready_queue) {
        PCB* ready = kernel_find_locked(pid);
        if (ready != nullptr && ready->state == ProcState::READY) {
            ready->waiting_time++;
        }
    }

    if (process->burst_remaining <= 0) {
        process->state = ProcState::TERMINATED;
        process->finish_time = g_kernel.clock_ticks + 1;
        g_kernel.running_pid = NO_PID;
    }

    gantt_record_locked(executed_pid);
}


// Elige el siguiente PID según Round Robin (frente de la cola).
int cpu_pick_next_rr_locked() {
    if (g_kernel.ready_queue.empty()) return NO_PID;

    const int pid = g_kernel.ready_queue.front();
    g_kernel.ready_queue.pop_front();
    PCB* process = kernel_find_locked(pid);
    if (process != nullptr) {
        process->state = ProcState::RUNNING;
    }
    return pid;
}

// Elige el siguiente PID según SRTF (menor ráfaga restante).
int cpu_pick_next_srtf_locked() {
    if (g_kernel.ready_queue.empty()) return NO_PID;

    auto best = g_kernel.ready_queue.begin();
    for (auto candidate = std::next(best); candidate != g_kernel.ready_queue.end(); ++candidate) {
        const PCB* current = kernel_find_locked(*best);
        const PCB* next = kernel_find_locked(*candidate);
        if (current == nullptr ||
            (next != nullptr &&
             (next->burst_remaining < current->burst_remaining ||
              (next->burst_remaining == current->burst_remaining &&
               (next->arrival_time < current->arrival_time ||
                (next->arrival_time == current->arrival_time && next->pid < current->pid)))))) {
            best = candidate;
        }
    }

    const int pid = *best;
    g_kernel.ready_queue.erase(best);
    PCB* process = kernel_find_locked(pid);
    if (process != nullptr) {
        process->state = ProcState::RUNNING;
    }
    return pid;
}
