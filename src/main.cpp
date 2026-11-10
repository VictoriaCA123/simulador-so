/**
 * @file main.cpp
 * @brief Orquestador: inicializa el kernel, lanza los hilos CLI y CPU,
 *        espera a que terminen y libera recursos.
 *
 * Flujo de apagado limpio:
 *   CLI recibe "exit" (o EOF/Ctrl+D) -> kernel_request_shutdown()
 *   -> el CPU despierta de cond_wait / sleep, ve exit_requested y sale
 *   -> main hace join de ambos hilos -> kernel_destroy().
 *
 * Responsable: Integrante 3.
 */
#include <pthread.h>

#include <cstring>
#include <iostream>

#include "cli.h"
#include "cpu.h"
#include "pcb.h"

int main() {
    if (!kernel_init()) {
        std::cerr << "[FATAL]: No se pudo inicializar el mutex del kernel.\n";
        return 1;
    }

    std::cout << "==================================================\n"
              << "  Simulador de Kernel - Consola y Motor de Planificación\n"
              << "  Escriba 'help' para ver los comandos.\n"
              << "==================================================\n";

    pthread_t cpu_tid;
    pthread_t cli_tid;

    int rc = pthread_create(&cpu_tid, nullptr, cpu_thread_main, nullptr);
    if (rc != 0) {
        std::cerr << "[FATAL]: No se pudo crear el hilo CPU: " << std::strerror(rc) << "\n";
        kernel_destroy();
        return 1;
    }

    rc = pthread_create(&cli_tid, nullptr, cli_thread_main, nullptr);
    if (rc != 0) {
        std::cerr << "[FATAL]: No se pudo crear el hilo CLI: " << std::strerror(rc) << "\n";
        kernel_request_shutdown();
        pthread_join(cpu_tid, nullptr);
        kernel_destroy();
        return 1;
    }

    pthread_join(cli_tid, nullptr);   // la CLI termina con "exit" o EOF
    kernel_request_shutdown();        // idempotente: por si la CLI no lo hizo
    pthread_join(cpu_tid, nullptr);

    kernel_destroy();
    std::cout << "[INFO]: Simulación finalizada. Hilos detenidos y recursos liberados.\n";
    return 0;
}
