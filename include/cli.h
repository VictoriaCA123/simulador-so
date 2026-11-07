/**
 * @file cli.h
 * @brief Hilo CLI: lectura de comandos, parsing estricto y validación.
 *
 * Responsable: Integrante 1 (Interfaz y Entradas).
 *
 * Contrato de robustez: NINGUNA entrada del usuario puede terminar el
 * programa ni dejar el mutex tomado. Ante cualquier error se imprime un
 * mensaje [ERROR ...] y se vuelve a pedir el siguiente comando.
 */
#ifndef CLI_H
#define CLI_H

#include <string>

/** Punto de entrada del hilo CLI (firma exigida por pthread_create). */
void* cli_thread_main(void* arg);

/**
 * Ejecuta UNA línea de comando ya leída.
 * @return false si el comando fue "exit" (el hilo debe terminar).
 */
bool cli_execute_line(const std::string& line);

/**
 * Carga un archivo de traza (comando load).
 * Formato por línea:  NOMBRE, BURST, PRIORIDAD, ARRIVAL
 * Líneas vacías y las que empiezan por '#' se ignoran.
 * @return cantidad de procesos cargados, o -1 si el archivo no se pudo abrir.
 */
int cli_load_workload(const std::string& path);

#endif // CLI_H
