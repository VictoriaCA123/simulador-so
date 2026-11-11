/**
 * @file report.cpp
 * @brief Comandos de visualización (ps, pcb, gantt, stats) y registro del Gantt.
 * 
 *
 * 
 *   
 *   
 *   
 * 
 */
#include "report.h"

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "pcb.h"

/** Nombre de un proceso para el Gantt ("IDLE" si no hubo proceso). REQUIERE lock. */
static std::string gantt_name_locked(int pid) {
    const PCB* p = kernel_find_locked(pid);
    return p != nullptr ? p->name : "IDLE";
}

/* --------------------------------- ps ---------------------------------- */

void report_ps() {
    std::vector<PCB> procesos;
    {
        KernelLock lock;
        procesos = g_kernel.table;   // copia bajo el lock
    }

    std::cout << std::left
              << std::setw(7)  << "PID"
              << std::setw(15) << "NOMBRE"
              << std::setw(13) << "ESTADO"
              << std::setw(18) << "BURST RESTANTE"
              << "PRIORIDAD" << "\n";
    std::cout << std::string(62, '-') << "\n";

    if (procesos.empty()) {
        std::cout << "(no hay procesos en el sistema)\n";
        return;
    }
    for (const PCB& p : procesos) {
        std::cout << std::left
                  << std::setw(7)  << p.pid
                  << std::setw(15) << p.name
                  << std::setw(13) << state_to_string(p.state)
                  << std::setw(18) << (std::to_string(p.burst_remaining) + "s")
                  << p.priority << "\n";
    }
}

/* --------------------------------- pcb --------------------------------- */

bool report_pcb(int pid) {
    PCB p;
    {
        KernelLock lock;
        const PCB* found = kernel_find_locked(pid);
        if (found == nullptr) return false;   // KernelLock libera al salir
        p = *found;                            // copia bajo el lock
    }

    const std::string linea(50, '=');
    std::cout << linea << "\n";
    std::cout << "        CONTROL BLOCK DE PROCESO (PCB) - PID: " << p.pid << "\n";
    std::cout << linea << "\n";
    std::cout << "Nombre del Proceso : " << p.name << "\n";
    std::cout << "Estado Actual      : " << state_to_string(p.state) << "\n";
    std::cout << "Prioridad          : " << p.priority << "\n";
    std::cout << "Tiempo de Llegada  : " << p.arrival_time << " ticks\n";
    std::cout << "Ráfaga Inicial     : " << p.burst_initial << " ticks\n";
    std::cout << "Ráfaga Restante    : " << p.burst_remaining << " ticks\n";
    std::cout << "Tiempo de Espera   : " << p.waiting_time << " ticks\n";

    if (p.finish_time >= 0) {
        std::cout << "Tiempo de Retorno  : " << (p.finish_time - p.arrival_time) << " ticks\n";
    } else if (p.state == ProcState::RUNNING) {
        std::cout << "Tiempo de Retorno  : N/A (En ejecución)\n";
    } else {
        std::cout << "Tiempo de Retorno  : N/A (No ha terminado)\n";
    }

    if (p.first_run_time >= 0) {
        std::cout << "Tiempo de Respuesta: " << (p.first_run_time - p.arrival_time) << " ticks\n";
    } else {
        std::cout << "Tiempo de Respuesta: N/A (Aún no usa la CPU)\n";
    }
    std::cout << linea << "\n";
    return true;
}

/* -------------------------------- gantt -------------------------------- */

void report_gantt() {
    struct Tramo { int start; int end; std::string name; bool idle; };
    std::vector<Tramo> tramos;
    {
        KernelLock lock;
        for (const GanttEntry& g : g_kernel.gantt) {
            tramos.push_back({g.start, g.end, gantt_name_locked(g.pid), g.pid == NO_PID});
        }
    }

    // El IDLE final (CPU sin trabajo hasta ahora) se muestra como [IDLE], sin tiempos.
    bool idle_final = !tramos.empty() && tramos.back().idle;
    if (idle_final) tramos.pop_back();

    int fin = tramos.empty() ? 0 : tramos.back().end;
    std::cout << "DIAGRAMA DE GANTT (0 - " << fin << " ticks):\n";

    if (tramos.empty() && !idle_final) {
        std::cout << "(la CPU aún no ha ejecutado ningún tick)\n";
        return;
    }
    for (size_t i = 0; i < tramos.size(); ++i) {
        if (i > 0) std::cout << " -> ";
        std::cout << "[" << tramos[i].start << "-" << tramos[i].end << "s: " << tramos[i].name << "]";
    }
    if (idle_final) std::cout << (tramos.empty() ? "" : " -> ") << "[IDLE]";
    std::cout << "\n";
}

/* -------------------------------- stats -------------------------------- */

void report_stats() {
    int total = 0, terminados = 0;
    double suma_wt = 0, suma_tat = 0, suma_rt = 0;
    int ultimo_fin = 0;
    int busy = 0;
    {
        KernelLock lock;
        total = static_cast<int>(g_kernel.table.size());
        busy = g_kernel.busy_ticks;
        for (const PCB& p : g_kernel.table) {
            if (p.state != ProcState::TERMINATED) continue;
            terminados++;
            suma_wt  += p.waiting_time;
            suma_tat += p.finish_time - p.arrival_time;
            suma_rt  += p.first_run_time - p.arrival_time;
        }
        // El uso de CPU se mide hasta el último tramo con trabajo, no hasta el
        // reloj actual: así los ticks IDLE del final no bajan el porcentaje.
        for (const GanttEntry& g : g_kernel.gantt) {
            if (g.pid != NO_PID) ultimo_fin = g.end;
        }
    }

    double avg_wt  = terminados > 0 ? suma_wt / terminados : 0.0;
    double avg_tat = terminados > 0 ? suma_tat / terminados : 0.0;
    double avg_rt  = terminados > 0 ? suma_rt / terminados : 0.0;
    double uso_cpu = ultimo_fin > 0 ? 100.0 * busy / ultimo_fin : 0.0;

    std::cout << "ESTADÍSTICAS DEL SISTEMA:\n";
    std::cout << "- Procesos Finalizados        : " << terminados << " / " << total << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "- Tiempo Promedio de Espera   : " << avg_wt << " ticks\n";
    std::cout << "- Tiempo Promedio de Retorno  : " << avg_tat << " ticks\n";
    std::cout << "- Tiempo Promedio de Respuesta: " << avg_rt << " ticks\n";
    std::cout << std::setprecision(1);
    std::cout << "- Uso de CPU                  : " << uso_cpu << "%\n";
    std::cout << std::defaultfloat << std::setprecision(6);   // restaurar formato de cout
}

/* ---------------------- registro del Gantt (hilo CPU) ---------------------- */

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
