#ifndef CPPXX_SQL_DML_H
#define CPPXX_SQL_DML_H

#include <cppxx/sql/core/statement.h>


/*
 * Data Manipulation Language
 */
namespace cppxx::sql {
    template <typename Table, typename... Cols>
    struct insert_into {
        static constexpr literal statement = literal("insert into ") + Table::_TableDefinition::name + literal(" (")
            + literal_join<", ", Cols::name...> + literal(") values (") + literal_join<", ", Cols::placeholder...>
            + literal(");");

        static constexpr auto values(const Cols::type &...vals) {
            return Statement<statement, typename Cols::type...>{{vals...}};
        }
    };

    template <typename T>
    struct update {
        static constexpr literal cmd = literal("update ") + T::_TableDefinition::name + literal(" ");

        template <typename... Statements>
        struct set {
            static constexpr literal set_cmd = cmd + literal("set ");
            constexpr set(Statements... statements)
                : statements{statements...} {}

            std::tuple<Statements...> statements;

            template <typename Condition>
            constexpr auto where(const Condition &condition) const {
                return Statement<set_cmd>{} + detail::statement_join_from_tuple<", ">(statements) + Statement<" where ">{} + condition
                    + Statement<";">{};
            }
        };
    };
} // namespace cppxx::sql

#endif
