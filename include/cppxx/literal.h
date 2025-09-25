#ifndef CPPXX_LITERAL_H
#define CPPXX_LITERAL_H

#include <optional>


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
            if constexpr (Start >= End) {
                return literal<1>("");
            } else {
                constexpr size_t len = (End > N ? N : End) - Start;
                char tmp[len + 1] = {};
                for (size_t i = 0; i < len; ++i)
                    tmp[i] = value[Start + i];
                return literal<len + 1>(tmp);
            }
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
    template <literal Lit, size_t Pos, size_t Skip>
    struct literal_split_into_two {
        static constexpr size_t N = Lit.size();
        static constexpr auto first = Lit.template slice<0, Pos>();
        static constexpr auto second = Lit.template slice<Pos + Skip, N>();
    };

    template <literal Lit, literal Token>
    struct literal_until {
        static constexpr size_t N = [] { return Lit.find(Token).value_or(Lit.size()); }();
        static constexpr auto value = literal_split_into_two<Lit, N, 0>::first;
    };
} // namespace cppxx::detail

namespace cppxx {
    template <literal Lit, literal Token>
    constexpr auto literal_until = detail::literal_until<Lit, Token>::value;

    template <literal Lit, literal Token>
    struct literal_split {
    private:
        static constexpr size_t pos = [] { return Lit.find(Token).value_or(Lit.size()); }();
        static constexpr size_t skip = [] { return Token.size(); }();
        using split = detail::literal_split_into_two<Lit, pos, skip>;
    public:
        static constexpr auto first = split::first;
        static constexpr auto second = split::second;
    };
}

#ifdef FMT_FORMAT_H_

template <size_t N>
struct fmt::formatter<cppxx::literal<N>> : fmt::formatter<std::string_view> {
    auto format(const cppxx::literal<N> &lit, fmt::context &ctx) const {
        return fmt::formatter<std::string_view>::format(lit.value, ctx);
    }
};

#endif
#endif
