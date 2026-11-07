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

/** Tabla de todos los procesos (formato sección 5 del enunciado). */
void report_ps();

/** Detalle del PCB. @return false si el PID no existe (la CLI imprime el error). */
bool report_pcb(int pid);

/** Diagrama de Gantt en texto: [0-2s: P1] -> [2-4s: P2] -> ... */
void report_gantt();

/** Promedios de WT, TAT, RT y % de uso de CPU. */
void report_stats();

/**
 * Registra que en el tick actual ejecutó `pid` (NO_PID = IDLE).
 * Si es el mismo pid que el último tramo, lo extiende en vez de crear otro.
 * La llama el hilo CPU dentro de cpu_tick_locked(). REQUIERE lock.
 */
void gantt_record_locked(int pid);

#endif // REPORT_H
