#ifndef CPPXX_SQL_H
#define CPPXX_SQL_H

#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <cppxx/sql/core/connection.h>
#include <cppxx/sql/core/format.h>
#include <cppxx/sql/core/ddl.h>
#include <cppxx/sql/core/dml.h>
#include <cppxx/sql/core/dql.h>


namespace cppxx::sql::detail {
    struct Bool {
        Bool(std::string value)
            : value(std::move(value)) {}

        operator const std::string &() const { return value; }

        Bool operator&&(const Bool &other) const { return '(' + value + " and " + other.value + ')'; }
        Bool operator||(const Bool &other) const { return '(' + value + " or " + other.value + ')'; }
        Bool operator!() const { return "not (" + value + ')'; }

        std::string value;
    };
} // namespace cppxx::sql::detail

namespace cppxx::sql {
    template <typename cpp_type, literal col>
    struct Column {
    private:
        using first_split = literal_split<col, " ">;
        using second_split = literal_split<first_split::second, " ">;

    public:
        using type = cpp_type;
        static constexpr literal lit = col;
        static constexpr literal name = first_split::first;
        static constexpr literal sql_type = second_split::first;
        static constexpr literal sql_constraint = second_split::second;

        std::string operator=(const type &val) const { return operator_<" = ">(val); }
        sql::detail::Bool operator==(const type &val) const { return operator_<" = ">(val); }
        sql::detail::Bool operator!=(const type &val) const { return operator_<" != ">(val); }
        sql::detail::Bool operator>(const type &val) const { return operator_<" > ">(val); }
        sql::detail::Bool operator<(const type &val) const { return operator_<" < ">(val); }
        sql::detail::Bool operator>=(const type &val) const { return operator_<" >= ">(val); }
        sql::detail::Bool operator<=(const type &val) const { return operator_<" <= ">(val); }

    private:
        template <literal op>
        std::string operator_(const type &val) const {
            static constexpr literal format_lit = name + op + format_col<type>;
            static constexpr fmt::format_string<const type &> format = format_lit.value;
            return fmt::format(format, val);
        }
    };

    template <literal TableName, typename... Cols>
    struct TableDefinition {
        static constexpr literal name = TableName;
        static constexpr literal content = literal("(") + literal_join<", ", Cols::lit...> + literal(")");
    };
} // namespace cppxx::sql

#endif
