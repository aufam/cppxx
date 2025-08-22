#ifndef CPPXX_MATCH_H
#define CPPXX_MATCH_H

#include <functional>
#include <initializer_list>
#include <utility>
#include <variant>

namespace cppxx {
    struct match_else {};
    struct match_pair {};

    template <typename R, typename T>
    R match(const T &what, std::initializer_list<std::pair<std::variant<T, match_else>, std::function<R()>>> &&patterns) {
        for (auto &[k, f] : patterns) {
            if ((std::holds_alternative<T>(k) && what == std::get<T>(k)) || std::holds_alternative<match_else>(k)) {
                return f();
            }
        }

        return {};
    }
} // namespace cppxx

#endif

