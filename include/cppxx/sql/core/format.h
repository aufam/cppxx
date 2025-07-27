#ifndef CPPXX_SQL_FORMAT_H
#define CPPXX_SQL_FORMAT_H

#include <cppxx/literal.h>


namespace cppxx::sql {
    template <typename T>
    static constexpr literal format_col = "{}";

    template <>
    constexpr literal format_col<std::string> = "'{}'";

    template <>
    constexpr literal format_col<const char *> = "'{}'";

    // maybe other types ?
    // template <typename T>
    // constexpr literal format_col<std::chrono::time_point> = "'{}'";
}

#endif
