#ifndef CPPXX_MATCH_H
#define CPPXX_MATCH_H

#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <variant>

namespace cppxx {
    struct match_else {};
    struct match_pair {};

    template <typename R, typename T>
    constexpr R
    match(const T &what, std::initializer_list<std::pair<std::variant<T, match_else>, std::function<R()>>> &&patterns) {
        for (auto &[k, f] : patterns) {
            if ((std::holds_alternative<T>(k) && what == std::get<T>(k)) || std::holds_alternative<match_else>(k)) {
                return f();
            }
        }

        throw std::runtime_error("no pattern matched");
    }

    template <typename T, typename F>
        requires std::invocable<F, T>
    struct pattern {
        using argtype = T;
        F fn = [](T &&t) { return std::move(t); };
    };

    template <typename... T, typename... Patterns>
    constexpr auto match_v2(const std::variant<T...> &what, Patterns... patterns) {
        using R = std::common_type_t<decltype(std::declval<Patterns>().fn(std::declval<typename Patterns::argtype>()))...>;

        bool matched = false;
        R result{};

        auto try_pattern = [&](auto &&p) {
            using VT = typename std::decay_t<decltype(what)>::argtype;
            if (!matched && std::holds_alternative<VT>(what)) {
                result = p.fn(std::get<VT>(what));
                matched = true;
            }
        };
        (try_pattern(std::forward<Patterns>(patterns)), ...);

        if (!matched)
            throw std::runtime_error("no pattern matched");

        return result;
    }
} // namespace cppxx

#endif
