#ifndef CPPXX_SQL_DDL_H
#define CPPXX_SQL_DDL_H

#include <cppxx/sql/core/statement.h>


/*
 * Data Definition Language
 */
namespace cppxx::sql {
    template <typename Table>
    static constexpr Statement<literal("create table ") + Table::_TableDefinition::name + literal(" ")
                               + Table::_TableDefinition::content + literal(";")>
        create_table = {};

    template <typename Table>
    static constexpr Statement<literal("create table if not exists ") + Table::_TableDefinition::name + literal(" ")
                               + Table::_TableDefinition::content + literal(";")>
        create_table_if_not_exists = {};

    template <typename Table>
    struct alter_table {
        static constexpr literal alter = literal("alter table ") + Table::_TableDefinition::name + literal(" ");

        template <typename Col>
        static constexpr Statement<alter + literal("add ") + Col::lit + literal(";")> add = {};

        template <typename Col>
        static constexpr Statement<alter + literal("drop column ") + Col::name + literal(";")> drop_column = {};

        template <typename ColFrom>
        struct rename_column {
            template <typename ColTo>
            static constexpr Statement<alter + literal("rename column ") + ColFrom::name + literal(" to ") + ColTo::name
                                       + literal(";")>
                to = {};
        };
    };

    template <typename Table>
    static constexpr Statement<literal("drop table ") + Table::_TableDefinition::name + literal(";")>
        drop_table = {};

    template <typename Table>
    static constexpr Statement<literal("truncate table ") + Table::_TableDefinition::name + literal(";")>
        truncate_table = {};
} // namespace cppxx::sql

#endif
