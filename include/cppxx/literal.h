#ifndef CPPXX_LITERAL_H
#define CPPXX_LITERAL_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <cppxx/tuple.h>


namespace cppxx {
    template <size_t N>
    struct literal {
        char value[N];
        constexpr literal(const char (&str)[N]) {
            for (size_t i = 0; i < N; ++i)
                value[i] = str[i];
        }

        template <size_t M>
        constexpr auto append(const literal<M> &other) const {
            char tmp[N + M - 1] = {};
            for (size_t i = 0; i < N - 1; ++i)
                tmp[i] = value[i];
            for (size_t i = 0; i < M; ++i)
                tmp[N - 1 + i] = other.value[i];
            return literal<sizeof(tmp)>(tmp);
        }

        template <size_t M>
        constexpr bool compare(const literal<M> &other) const {
            if constexpr (M == N) {
                for (size_t i = 0; i < N; ++i)
                    if (value[i] != other.value[i])
                        return false;
                return true;
            } else
                return false;
        }

        template <size_t M>
        constexpr std::optional<size_t> find(const literal<M> &other) const {
            if constexpr (M == 0 || M > N)
                return std::nullopt;

            for (size_t i = 0; i <= N - M; ++i) {
                bool match = true;
                for (size_t j = 0; j < M - 1; ++j) {
                    if (value[i + j] != other.value[j]) {
                        match = false;
                        break;
                    }
                }
                if (match)
                    return i;
            }
            return std::nullopt;
        }

        template <size_t M>
        constexpr bool contains(const literal<M> &other) const {
            return find(other).has_value();
        }

        constexpr size_t size() const { return N - 1; }
        constexpr char at(size_t i) const { return value[i]; }

        template <size_t Start, size_t End>
        constexpr auto slice() const {
            constexpr size_t len = End - Start;
            char tmp[len + 1] = {};
            for (size_t i = 0; i < len; ++i)
                tmp[i] = value[Start + i];
            return literal<len + 1>(tmp);
        }

        template <char c>
        constexpr size_t count_char() const {
            size_t cnt = 0;
            for (size_t i = 0; i < N - 1; ++i)
                cnt += value[i] == c;
            return cnt;
        }
    };

    template <size_t N, size_t M>
    constexpr bool operator==(const literal<N> &lhs, const literal<M> &rhs) {
        return lhs.compare(rhs);
    }

    template <size_t N, size_t M>
    constexpr auto operator+(const literal<N> &lhs, const literal<M> &rhs) {
        return lhs.append(rhs);
    }


#if __cplusplus > 202000L
    template <literal L>
    struct literal_type;

    template <literal L>
    using literal_type_t = typename literal_type<L>::type;
#elif __cplusplus > 201700L
    template <typename L>
    struct literal_type;

    template <typename L>
    using literal_type_t = typename literal_type<L>::type;
#endif

    template <>
    struct literal_type<"double"> {
        using type = double;
    };
    template <>
    struct literal_type<"float"> {
        using type = float;
    };
    template <>
    struct literal_type<"int32"> {
        using type = int32_t;
    };
    template <>
    struct literal_type<"int64"> {
        using type = int64_t;
    };
    template <>
    struct literal_type<"uint32"> {
        using type = uint32_t;
    };
    template <>
    struct literal_type<"uint64"> {
        using type = uint64_t;
    };
    template <>
    struct literal_type<"sint32"> {
        using type = int32_t;
    };
    template <>
    struct literal_type<"sint64"> {
        using type = int64_t;
    };
    template <>
    struct literal_type<"fixed32"> {
        using type = uint32_t;
    };
    template <>
    struct literal_type<"fixed64"> {
        using type = uint64_t;
    };
    template <>
    struct literal_type<"sfixed32"> {
        using type = int32_t;
    };
    template <>
    struct literal_type<"sfixed64"> {
        using type = int64_t;
    };
    template <>
    struct literal_type<"bool"> {
        using type = bool;
    };
    template <>
    struct literal_type<"string"> {
        using type = std::string;
    };
    template <>
    struct literal_type<"bytes"> {
        using type = std::vector<uint8_t>;
    };
} // namespace cppxx


/*
 * Literal join
 */

namespace cppxx::detail {
    template <literal Separator>
    constexpr auto literal_join() {
        return literal("");
    }

    template <literal Separator, literal First>
    constexpr auto literal_join() {
        return First;
    }

    template <literal Separator, literal First, literal Second, literal... Rest>
    constexpr auto literal_join() {
        return First + Separator + literal_join<Separator, Second, Rest...>();
    }
} // namespace cppxx::detail

namespace cppxx {
    template <literal Separator, literal... Lits>
    constexpr literal literal_join = cppxx::detail::literal_join<Separator, Lits...>();
}

/*
 * Literal until
 */
namespace cppxx::detail {
    template <literal Lit, literal Token, size_t Pos>
    struct literal_until_impl {
        static constexpr auto value = Lit.template slice<0, Pos>();
    };

    template <literal Lit, literal Token>
    struct literal_until {
        static constexpr size_t N = [] {
            return Lit.find(Token).value_or(Lit.size());
        }();

        static constexpr auto value = literal_until_impl<Lit, Token, N>::value;
    };
}

namespace cppxx {
    template <literal Lit, literal Token>
    constexpr auto literal_until = detail::literal_until<Lit, Token>::value;
}

#endif
