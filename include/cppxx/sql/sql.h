#ifndef CPPXX_SQL_H
#define CPPXX_SQL_H

#include <cppxx/sql/core/connection.h>
#include <cppxx/sql/core/statement.h>
#include <cppxx/sql/core/ddl.h>
#include <cppxx/sql/core/dml.h>
#include <cppxx/sql/core/dql.h>


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
        static constexpr literal placeholder = "?";

        auto operator=(const type &val) const { return Statement<name + literal(" = ?"), type>{{val}}; }
        auto operator==(const type &val) const { return Statement<name + literal(" = ?"), type>{{val}}; }
        auto operator!=(const type &val) const { return Statement<name + literal(" != ?"), type>{{val}}; }
        auto operator>(const type &val) const { return Statement<name + literal(" > ?"), type>{{val}}; }
        auto operator<(const type &val) const { return Statement<name + literal(" < ?"), type>{{val}}; }
        auto operator>=(const type &val) const { return Statement<name + literal(" >= ?"), type>{{val}}; }
        auto operator<=(const type &val) const { return Statement<name + literal(" <= ?"), type>{{val}}; }
    };

    template <literal TableName, typename... Cols>
    struct TableDefinition {
        static constexpr literal name = TableName;
        static constexpr literal content = literal("(") + literal_join<", ", Cols::lit...> + literal(")");
    };
} // namespace cppxx::sql

#endif
