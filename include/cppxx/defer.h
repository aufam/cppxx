#ifndef CPPXX_DEFER_H
#define CPPXX_DEFER_H

#include <utility>

namespace cppxx {
    template <typename F>
    class defer {
        F fn;

    public:
        defer(F fn)
            : fn(std::move(fn)) {}

        ~defer() { fn(); }
    };
} // namespace cppxx

#endif
