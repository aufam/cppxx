#ifndef CPPXX_SQL_DDL_H
#define CPPXX_SQL_DDL_H

#include <cppxx/literal.h>


/*
 * Data Definition Language
 */
namespace cppxx::sql {
    template <typename T>
    static constexpr literal create_table =
        literal("create table ") + T::_TableDefinition::name + literal(" ") + T::_TableDefinition::content + literal(";");

    template <typename T>
    static constexpr literal create_table_if_not_exists =
        literal("create table if not exists ") + T::_TableDefinition::name + literal(" ") + T::_TableDefinition::content + literal(";");

    template <typename T>
    struct alter_table {
        static constexpr literal alter = literal("alter table ") + T::_TableDefinition::name + literal(" ");

        template <typename C>
        static constexpr literal add = alter + literal("add ") + C::lit + literal(";");

        template <typename C>
        static constexpr literal drop_column = alter + literal("drop column ") + C::name + literal(";");

        template <typename CFrom>
        struct rename_column {
            template <typename CTo>
            static constexpr literal to =
                alter + literal("rename column ") + CFrom::name + literal(" to ") + CTo::name + literal(";");
        };
    };

    template <typename T>
    constexpr literal drop_table = literal("drop table ") + T::_TableDefinition::name + literal(";");

    template <typename T>
    constexpr literal truncate_table = literal("truncate table ") + T::_TableDefinition::name + literal(";");
} // namespace cppxx::sql

#endif
