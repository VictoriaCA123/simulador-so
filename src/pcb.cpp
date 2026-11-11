/**
 * @file pcb.cpp
 * @brief Implementación del estado global del kernel y su API básica.
 * Responsable: Integrante 3.
 */
#include "../include/pcb.h"

PCB createProcess(int pid, const std::string& name, int burst_time, int priority, int arrival_time) {
    PCB p;
    p.pid = pid;
    p.name = name;
    p.state = ProcessState::NEW;
    p.burst_time = burst_time;
    p.remaining_time = burst_time;
    p.priority = priority;
    p.arrival_time = arrival_time;
    
    // Inicialización de métricas en 0 o valores neutros
    p.waiting_time = 0;
    p.turnaround_time = 0;
    p.response_time = -1;
    p.has_responded = false;
    p.completion_time = 0;
    
    return p;
}