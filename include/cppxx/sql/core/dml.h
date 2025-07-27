#ifndef CPPXX_SQL_DML_H
#define CPPXX_SQL_DML_H

#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <cppxx/sql/core/format.h>

/*
 * Data Manipulation Language
 */
namespace cppxx::sql {
    template <typename T>
    struct insert_into {
        static constexpr literal cmd = literal("insert into ") + T::_TableName + literal(" ");

        template <typename... C>
        static std::string values(const C::type &...vals) {
            static constexpr literal format_lit = cmd + literal("(") + literal_join<", ", C::name...>
                + literal(") values (") + literal_join<", ", format_col<typename C::type>...> + literal(");");
            static constexpr fmt::format_string<const typename C::type &...> format = format_lit.value;
            return fmt::format(format, vals...);
        }
    };

    template <typename T>
    struct update {
        static constexpr literal cmd = literal("update ") + T::_TableName + literal(" ");

        struct set {
            static constexpr literal set_format = cmd + literal("set {} ");
            std::string setters;

            set(std::vector<std::string> vec)
                : setters(fmt::format("{}", fmt::join(vec, ","))) {}

            std::string where(const std::string& filters) const {
                static constexpr literal format_lit = set_format + literal("where {};");
                static constexpr fmt::format_string<const std::string &, const std::string &> format = format_lit.value;
                return fmt::format(format, setters, filters);
            }
        };
    };
} // namespace cppxx::sql

#endif
