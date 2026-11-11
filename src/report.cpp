/**
 * @file report.cpp
 * @brief Comandos de visualización y métricas.
 * Responsable: Integrante 3.
 */
#include "../include/report.h"
#include <iostream>
#include <iomanip>

// 1. Salida del comando ps
void ReportManager::printPS(const std::vector<PCB>& processes) {
    std::cout << std::left 
              << std::setw(6) << "PID" 
              << std::setw(15) << "NOMBRE" 
              << std::setw(14) << "ESTADO" 
              << std::setw(18) << "BURST RESTANTE" 
              << std::setw(10) << "PRIORIDAD" << "\n";

    for (const auto& p : processes) {
        std::cout << std::left 
                  << std::setw(6) << p.pid
                  << std::setw(15) << p.name
                  << std::setw(14) << stateToString(p.state)
                  << std::setw(18) << (std::to_string(p.remaining_time) + "s")
                  << std::setw(10) << p.priority << "\n";
    }
}

// 2. Salida del comando pcb <PID>
void ReportManager::printPCBDetails(const PCB& p) {
    std::cout << "====================================================\n";
    std::cout << "       CONTROL BLOCK DE PROCESO (PCB) - PID: " << p.pid << "       \n";
    std::cout << "====================================================\n";
    std::cout << "Nombre del Proceso : " << p.name << "\n";
    std::cout << "Estado Actual      : " << stateToString(p.state) << "\n";
    std::cout << "Prioridad          : " << p.priority << "\n";
    std::cout << "Tiempo de Llegada  : " << p.arrival_time << " ticks\n";
    std::cout << "Ráfaga Inicial     : " << p.burst_time << " ticks\n";
    std::cout << "Ráfaga Restante    : " << p.remaining_time << " ticks\n";
    std::cout << "Tiempo de Espera   : " << p.waiting_time << " ticks\n";
    
    if (p.state == ProcessState::TERMINATED) {
        std::cout << "Tiempo de Retorno  : " << p.turnaround_time << " ticks\n";
    } else {
        std::cout << "Tiempo de Retorno  : N/A (En ejecución)\n";
    }
    
    std::cout << "Tiempo de Respuesta: " << (p.has_responded ? std::to_string(p.response_time) + " ticks" : "N/A") << "\n";
    std::cout << "====================================================\n";
}

// 3. Salida del comando gantt
void ReportManager::printGantt(const std::vector<GanttEntry>& timeline, int current_ticks) {
    std::cout << "DIAGRAMA DE GANTT (0 - " << current_ticks << " ticks):\n";
    for (size_t i = 0; i < timeline.size(); ++i) {
        std::cout << "[" << timeline[i].start_time << "-" << timeline[i].end_time << "s: " << timeline[i].process_name << "]";
        if (i + 1 < timeline.size()) {
            std::cout << " -> ";
        }
    }
    std::cout << "\n";
}

// 4. Salida del comando stats
void ReportManager::printStats(const std::vector<PCB>& processes, float cpu_utilization, int total_ticks) {
    int terminated_count = 0;
    float total_wt = 0;
    float total_tat = 0;
    int count = processes.size();

    for (const auto& p : processes) {
        if (p.state == ProcessState::TERMINATED) {
            terminated_count++;
            total_wt += p.waiting_time;
            total_tat += p.turnaround_time;
        }
    }

    float avg_wt = (terminated_count > 0) ? (total_wt / terminated_count) : 0.0f;
    float avg_tat = (terminated_count > 0) ? (total_tat / terminated_count) : 0.0f;

    std::cout << "ESTADÍSTICAS DEL SISTEMA:\n";
    std::cout << "Procesos Finalizados        : " << terminated_count << "/" << count << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Tiempo Promedio de Espera   : " << avg_wt << " ticks\n";
    std::cout << "Tiempo Promedio de Retorno  : " << avg_tat << " ticks\n";
    std::cout << "Uso de CPU                  : " << cpu_utilization << "%\n";
}