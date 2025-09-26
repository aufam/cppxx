#ifndef CPPXX_SQL_H
#define CPPXX_SQL_H

#include "literal.h"
#include "tuple.h"

namespace cppxx::sql::detail {
    template <size_t i>
    struct repeated_placeholders {
        static constexpr literal value = repeated_placeholders<i - 1>::value + literal(", ?");
    };

    template <>
    struct repeated_placeholders<0> {
        static constexpr literal value = "";
    };

    template <>
    struct repeated_placeholders<1> {
        static constexpr literal value = "?";
    };
} // namespace cppxx::sql::detail

namespace cppxx::sql {
    /// Database connection abstract class
    class Connection {
    public:
        virtual ~Connection() = default;
    };

    /// Database rows abstract class
    template <tuple_like Row>
    class Rows {
    public:
        virtual void next() = 0;
        virtual bool is_done() const = 0;
        virtual Row get() const = 0;
        virtual ~Rows() = default;
    };

    /// SQL Statement
    template <literal Stmt, tuple_like Params = std::tuple<>, tuple_like Row = std::tuple<>>
    struct Statement {
        static constexpr literal stmt = Stmt;
        using params_type = Params;
        using row_type = Row;

        Params params;

        template <typename Other>
        constexpr auto operator+(const Other &other) const {
            return Statement<stmt + Other::stmt, decltype(std::tuple_cat(params_type{}, typename Other::params_type{})),
                             decltype(std::tuple_cat(row_type{}, typename Other::row_type{}))>{
                std::tuple_cat(params, other.params)};
        }

        template <typename Other>
        constexpr auto operator&&(const Other &other) const {
            return Statement<"(">{} + *this + Statement<" and ">{} + other + Statement<")">{};
        }

        template <typename Other>
        constexpr auto operator||(const Other &other) const {
            return Statement<"(">{} + *this + Statement<" or ">{} + other + Statement<")">{};
        }

        constexpr auto operator!() const { return Statement<"not (">{} + *this + Statement<")">{}; }

        template <typename Other, typename... Rest>
        constexpr auto set(const Other &other, const Rest &...rest) const {
            if constexpr (sizeof...(Rest) == 0) {
                return *this + Statement<" set ">{} + other;
            } else {
                return *this + Statement<" set ">{} + other + ((Statement<", ">{} + rest) + ...);
            }
        }

        template <typename Table>
        constexpr auto from(const Table &) const {
            return *this + Statement<literal(" from ") + Table::Schema::name>{};
        }

        template <typename Col>
        constexpr auto to(const Col &) const {
            return *this + Statement<literal(" to ") + Col::name>{};
        }

        template <typename Col>
        constexpr auto add(const Col &) const {
            return *this + Statement<literal(" add ") + Col::name>{};
        }

        template <typename Col>
        constexpr auto drop_column(const Col &) const {
            return *this + Statement<literal(" drop column ") + Col::name>{};
        }

        template <literal alias>
        constexpr auto as() const {
            return *this + Statement<literal(" as ") + alias>{};
        }

        template <typename Condition>
        constexpr auto where(const Condition &condition) const {
            return *this + Statement<" where ">{} + condition;
        }

        constexpr auto operator()() const { return *this + Statement<literal(" ()")>{}; }

        template <typename Col, typename... Cols>
        constexpr auto operator()(const Col &, const Cols &...) const {
            if constexpr (sizeof...(Cols) == 0) {
                return *this + Statement<literal(" (") + Col::name, std::tuple<typename Col::type>>{} + Statement<")">{};
            } else {
                return *this + Statement<literal(" (") + Col::name, std::tuple<typename Col::type>>{}
                + (Statement<literal(", ") + Cols::name, std::tuple<typename Cols::type>>{} + ...) + Statement<")">{};
            }
        }

        constexpr auto values(const Params &params) const {
            return Statement<Stmt + literal(" values (") + detail::repeated_placeholders<std::tuple_size_v<Params>>::value
                                 + literal(")"),
                             Params, Row>{params};
        }
    };

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

        auto operator=(const type &val) const { return Statement<name + literal(" = ?"), std::tuple<type>>{{val}}; }
        auto operator==(const type &val) const { return Statement<name + literal(" = ?"), std::tuple<type>>{{val}}; }
        auto operator!=(const type &val) const { return Statement<name + literal(" != ?"), std::tuple<type>>{{val}}; }
        auto operator>(const type &val) const { return Statement<name + literal(" > ?"), std::tuple<type>>{{val}}; }
        auto operator<(const type &val) const { return Statement<name + literal(" < ?"), std::tuple<type>>{{val}}; }
        auto operator>=(const type &val) const { return Statement<name + literal(" >= ?"), std::tuple<type>>{{val}}; }
        auto operator<=(const type &val) const { return Statement<name + literal(" <= ?"), std::tuple<type>>{{val}}; }
    };

    /// Schema definition
    template <literal TableName, typename... Cols>
    struct Schema {
        static constexpr literal name = TableName;
        static constexpr literal columns = literal("(") + literal_join<", ", Cols::lit...> + literal(")");
    };

    template <typename Table>
    static constexpr Statement<literal("create table ") + Table::Schema::name + literal(" ") + Table::Schema::columns>
        create_table = {};

    template <typename Table>
    static constexpr Statement<literal("create table if not exists ") + Table::Schema::name + literal(" ")
                               + Table::Schema::columns>
        create_table_if_not_exists = {};

    template <typename Table>
    static constexpr Statement<literal("alter table ") + Table::Schema::name> alter_table = {};

    template <typename Table>
    static constexpr Statement<literal("drop table ") + Table::Schema::name> drop_table = {};

    template <typename Table>
    static constexpr Statement<literal("truncate table ") + Table::Schema::name> truncate_table = {};

    template <typename Table>
    static constexpr auto insert_into = Statement<literal("insert into ") + Table::Schema::name>{};

    template <typename Table>
    static constexpr auto update = Statement<literal("update ") + Table::Schema::name>{};

    template <typename Col, typename... Cols>
    static constexpr auto select = [] {
        if constexpr (sizeof...(Cols) == 0) {
            return Statement<literal("select ") + Col::name, std::tuple<>, std::tuple<typename Col::type>>{};
        } else {
            return Statement<literal("select ") + Col::name + ((literal(", ") + Cols::name) + ...), std::tuple<>,
                             std::tuple<typename Col::type, typename Cols::type...>>{};
        }
    }();
} // namespace cppxx::sql

#endif
