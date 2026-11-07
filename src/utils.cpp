/**
 * @file utils.cpp
 * @brief Parsing defensivo. Ver utils.h.
 */
#include "utils.h"
#include "pcb.h"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <sstream>

Args tokenize(const std::string& line) {
    Args tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

ParseStatus parse_int(const std::string& s, int min, int max, int& out) {
    if (s.empty()) return ParseStatus::NOT_INTEGER;

    // 1) Validación manual de caracteres: [+-]?[0-9]+
    size_t i = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (i == s.size()) return ParseStatus::NOT_INTEGER;          // solo "+" o "-"
    for (size_t j = i; j < s.size(); ++j) {
        if (!std::isdigit(static_cast<unsigned char>(s[j]))) return ParseStatus::NOT_INTEGER;
    }

    // 2) Conversión con detección de desborde
    errno = 0;
    char* end = nullptr;
    long long v = std::strtoll(s.c_str(), &end, 10);
    if (errno == ERANGE || *end != '\0') return ParseStatus::OUT_OF_RANGE;
    if (v < min || v > max) return ParseStatus::OUT_OF_RANGE;

    out = static_cast<int>(v);
    return ParseStatus::OK;
}

bool is_valid_name(const std::string& name) {
    if (name.empty() || name.size() > static_cast<size_t>(MAX_NAME_LEN)) return false;
    for (unsigned char c : name) {
        if (!(std::isalnum(c) || c == '_' || c == '-' || c == '.')) return false;
    }
    return true;
}

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}
