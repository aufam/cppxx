#ifndef CPPXX_SQL_DQL_H
#define CPPXX_SQL_DQL_H

#include <cppxx/sql/core/statement.h>

/*
 * Data Query Language
 */
namespace cppxx::sql {
    template <typename... Cols>
    struct select {
        template <typename Table>
        struct from {};
    };

    template <typename Table>
    struct select_all_from {};
} // namespace cppxx::sql

#endif
