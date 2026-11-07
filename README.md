# Simulador de Kernel y Gestor de Procesos (Fase I)

Shell interactiva + motor de planificación (RR / SRTF) con hilos POSIX.

## Compilar y ejecutar
```bash
make          # compila ./simulador
./simulador   # ejecuta
make debug    # AddressSanitizer + UBSan: detecta segfaults y accesos inválidos
make tsan     # ThreadSanitizer: detecta condiciones de carrera
make clean
```

## Pruebas rápidas
```bash
./simulador < tests/ataques.txt      # entradas maliciosas: no debe colapsar
```
Escenario batch: `set_sched SRTF`, `load tests/workload_test.txt`, `start`, `gantt`.

## Estructura
| Archivo | Responsable | Contenido |
|---|---|---|
| `include/pcb.h`, `src/pcb.cpp` | Integrante 3 | PCB, colas, mutex, `KernelLock` |
| `src/main.cpp` | Integrante 3 | Crea/une los hilos, apagado limpio |
| `include/report.h`, `src/report.cpp` | Integrante 3 | `ps`, `pcb`, `gantt`, `stats` |
| `include/cli.h`, `src/cli.cpp` | Integrante 1 | Hilo CLI, validación, `load` |
| `include/utils.h`, `src/utils.cpp` | Integrante 1 | `parse_int`, `tokenize`, `trim` |
| `include/cpu.h`, `src/cpu.cpp` | Integrante 2 | Hilo CPU, RR, SRTF |

## Reglas del equipo
1. Todo acceso a `g_kernel` va dentro de `{ KernelLock lock; ... }`.
2. Funciones `*_locked` asumen el lock tomado: no volver a tomarlo dentro.
3. Las colas guardan PIDs, nunca punteros.
4. Nada de `sleep`/`getline` con el lock tomado. El hilo CPU no imprime.
5. Prohibido `atoi`, `stoi`, `cin >> int`: usar `parse_int()`.
6. Cada uno trabaja en su rama y abre Pull Request a `main`.
