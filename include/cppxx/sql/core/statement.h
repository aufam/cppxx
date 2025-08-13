#ifndef CPPXX_SQL_STATEMENT_H
#define CPPXX_SQL_STATEMENT_H

#include <cppxx/literal.h>


namespace cppxx::sql {
    /// Represents a SQL statement with parameter placeholders.
    /// @tparam _Statement The SQL string as a `literal`
    /// @tparam ValueTypes The types of parameters that will be bound to `?` placeholders
    template <literal _Statement, typename... Placeholders>
    struct Statement {
        static constexpr literal value = _Statement;
        static constexpr bool is_valid = value.template count_char<'?'>() == sizeof...(Placeholders);

        std::tuple<Placeholders...> placeholders;


        template <literal OtherStatement, typename... OtherPlaceholders>
        constexpr auto operator&&(const Statement<OtherStatement, OtherPlaceholders...> &other) const {
            return Statement<literal("(") + _Statement + literal(" and ") + OtherStatement + literal(")"),
                             Placeholders...,
                             OtherPlaceholders...>{std::tuple_cat(placeholders, other.placeholders)};
        }

        template <literal OtherStatement, typename... OtherPlaceholders>
        constexpr auto operator||(const Statement<OtherStatement, OtherPlaceholders...> &other) const {
            return Statement<literal("(") + _Statement + literal(" or ") + OtherStatement + literal(")"),
                             Placeholders...,
                             OtherPlaceholders...>{std::tuple_cat(placeholders, other.placeholders)};
        }

        constexpr auto operator!() const {
            return Statement<literal("not (") + _Statement + literal(")"), Placeholders...>{placeholders};
        }

        template <literal OtherStatement, typename... OtherPlaceholders>
        constexpr auto operator+(const Statement<OtherStatement, OtherPlaceholders...> &other) const {
            return Statement<_Statement + OtherStatement, Placeholders..., OtherPlaceholders...>{
                std::tuple_cat(placeholders, other.placeholders)};
        }
    };
} // namespace cppxx::sql


namespace cppxx::sql::detail {
    template <literal Separator>
    constexpr auto statement_join() {
        return Statement<"">{};
    }

    template <literal Separator, typename First>
    constexpr auto statement_join(const First &first) {
        return first;
    }

    template <literal Separator, typename First, typename Second, typename... Rest>
    constexpr auto statement_join(const First &first, const Second &second, const Rest &...rest) {
        return first + Statement<Separator>{} + statement_join<Separator, Second, Rest...>(second, rest...);
    }

    template <literal Separator, typename... Statements>
    constexpr auto statement_join_from_tuple(const std::tuple<Statements...> &statements) {
        return [&statements]<std::size_t... I>(std::index_sequence<I...>) {
            return statement_join<Separator>(std::get<I>(statements)...);
        }(std::index_sequence_for<Statements...>{});
    }
} // namespace cppxx::sql::detail

#endif
