/**
 * @file cpu.h
 * @brief Hilo Motor de Scheduling: reloj simulado, RR y SRTF.
 *
 * Responsable: Integrante 2 (Algoritmos).
 *
 * Convención de tiempo: en cada iteración del hilo CPU con reloj = t
 *   1) cpu_admit_arrivals_locked()  -> NEW con arrival_time <= t pasan a READY
 *   2) cpu_tick_locked()            -> se ejecuta el intervalo [t, t+1)
 *   3) clock_ticks pasa a t+1       (lo hace cpu_thread_main)
 * Un proceso que termina en ese tick tiene finish_time = t + 1.
 */
#ifndef CPU_H
#define CPU_H

/** Punto de entrada del hilo CPU (firma exigida por pthread_create). */
void* cpu_thread_main(void* arg);

/** Mueve a READY los procesos NEW cuyo arrival_time ya llegó. REQUIERE lock. */
void cpu_admit_arrivals_locked();

/**
 * Simula UN tick de CPU con la política activa (g_kernel.algo):
 * elige proceso, aplica desapropiación, descuenta ráfaga, suma espera a los
 * READY, marca TERMINATED y registra el tick en el Gantt. REQUIERE lock.
 */
void cpu_tick_locked();

/** Siguiente PID según Round Robin (frente de la cola). NO_PID si vacía. REQUIERE lock. */
int cpu_pick_next_rr_locked();

/** Siguiente PID según SRTF (menor ráfaga restante). NO_PID si vacía. REQUIERE lock. */
int cpu_pick_next_srtf_locked();

#endif // CPU_H
