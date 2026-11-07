/**
 * @file report.cpp
 * @brief Comandos de visualización y métricas.
 * Responsable: Integrante 3.
 */
#include "report.h"

#include <iostream>

#include "pcb.h"

void report_ps() {
    // TODO (Integrante 3): copiar g_kernel.table bajo el lock y luego imprimir
    // con el formato exacto de la sección 5 (usar std::setw de <iomanip>).
    KernelLock lock;
    std::cout << "[TODO] ps: hay " << g_kernel.table.size()
              << " proceso(s) en la tabla. Reloj = " << g_kernel.clock_ticks << " ticks.\n";
}

bool report_pcb(int pid) {
    KernelLock lock;
    PCB* p = kernel_find_locked(pid);
    if (p == nullptr) return false;
    // TODO (Integrante 3): formato completo de la sección 5 (WT, TAT, RT; "N/A" si no aplica).
    std::cout << "[TODO] pcb: PID " << p->pid << " (" << p->name << ") estado "
              << state_to_string(p->state) << "\n";
    return true;
}

void report_gantt() {
    // TODO (Integrante 3): DIAGRAMA DE GANTT (0 - N ticks):
    // [0-2s: P1] -> [2-4s: P2] -> ... -> [IDLE]
    KernelLock lock;
    std::cout << "[TODO] gantt: " << g_kernel.gantt.size() << " tramo(s) registrados.\n";
}

void report_stats() {
    // TODO (Integrante 3): promedios WT/TAT/RT de los TERMINATED y
    // uso de CPU = busy_ticks / clock_ticks * 100 (cuidado con dividir entre 0).
    std::cout << "[TODO] stats\n";
}

void gantt_record_locked(int pid) {
    int t = g_kernel.clock_ticks;
    if (!g_kernel.gantt.empty()) {
        GanttEntry& last = g_kernel.gantt.back();
        if (last.pid == pid && last.end == t) {   // mismo proceso seguido: extender
            last.end = t + 1;
            return;
        }
    }
    g_kernel.gantt.push_back({t, t + 1, pid});
}
