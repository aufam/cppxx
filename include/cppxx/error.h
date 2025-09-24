#ifndef CPPXX_ERROR_H
#define CPPXX_ERROR_H

#include <stdexcept>
#include <expected>


namespace cppxx {
    using error = std::runtime_error;

    template <typename T>
    using Result = std::expected<T, error>;

    template <typename T, typename E>
    [[nodiscard]] std::unexpected<E> unexpected_move(std::expected<T, E> &r) {
        return std::unexpected(std::move(r.error()));
    }

    template <typename T, typename E>
    T try_(std::expected<T, E> r) {
        if (!r)
            throw std::move(r.error());
        return std::move(*r);
    }
} // namespace cppxx

#if defined(FMT_RANGES_H_)
namespace cppxx {
    template <typename... T>
    error errorf(fmt::format_string<T...> fmt, T &&...args) {
        return error(fmt::format(fmt, std::forward<T>(args)...));
    }

    template <typename... T>
    std::unexpected<error> unexpected_errorf(fmt::format_string<T...> fmt, T &&...args) {
        return std::unexpected(error(fmt::format(fmt, std::forward<T>(args)...)));
    }
} // namespace cppxx
#endif
#endif
