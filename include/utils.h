/**
 * @file utils.h
 * @brief Utilidades de parsing a prueba de entradas maliciosas.
 *
 * Regla del equipo: PROHIBIDO usar atoi(), stoi() o cin >> int.
 *   - atoi("abc") devuelve 0 sin avisar.
 *   - stoi("abc") lanza una excepción que, sin try/catch, mata el programa.
 *   - cin >> int deja el stream en estado de error y la CLI entra en bucle.
 * Usar siempre parse_int().
 */
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

using Args = std::vector<std::string>;

enum class ParseStatus { OK, NOT_INTEGER, OUT_OF_RANGE };

/** Divide una línea por espacios/tabs (ignora espacios repetidos). */
Args tokenize(const std::string& line);

/**
 * Convierte s a entero SOLO si es un entero decimal completo y está en
 * [min, max]. Rechaza "", "abc", "5abc", "3.5", "1e3", " 7", desbordes, etc.
 */
ParseStatus parse_int(const std::string& s, int min, int max, int& out);

/** Nombre válido: 1..MAX_NAME_LEN caracteres de [A-Za-z0-9_-.]. */
bool is_valid_name(const std::string& name);

/** Quita espacios, tabs y \r al inicio y al final. */
std::string trim(const std::string& s);

#endif // UTILS_H
