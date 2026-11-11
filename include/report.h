/**
 * @file report.h
 * @brief Visualización (ps, pcb, gantt, stats) y registro de métricas.
 *
 * Responsable: Integrante 3 (Concurrencia, Estado y Métricas).
 *
 * Las funciones report_* son llamadas desde el hilo CLI y toman el lock
 * ellas mismas. Recomendado: copiar los datos bajo el lock y luego imprimir.
 */
#ifndef REPORT_H
#define REPORT_H

#include "pcb.h"
#include <vector>
#include <string>

// Estructura para registrar los intervalos del Diagrama de Gantt
struct GanttEntry {
    int start_time;
    int end_time;
    std::string process_name;
};

class ReportManager {
public:
    static void printPS(const std::vector<PCB>& processes);
    static void printPCBDetails(const PCB& p);
    static void printGantt(const std::vector<GanttEntry>& timeline, int current_ticks);
    static void printStats(const std::vector<PCB>& processes, float cpu_utilization, int total_ticks);
};

#endif // REPORT_H