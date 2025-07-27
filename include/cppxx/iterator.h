#ifndef CPPXX_ITERATOR_H
#define CPPXX_ITERATOR_H

#include <concepts>
#include <limits>
#include <ranges>
#include <type_traits>
#include <utility>
#include <optional>
#include <expected>
#include <cppxx/tuple.h>


namespace cppxx {
    using std::views::zip;
    using std::views::repeat;
    using std::views::single;
    using std::views::join_with;


    struct iter {
        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const iter &) {
            return std::views::all(std::forward<R>(r));
        }
    };

    struct reverse {
        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const reverse &) {
            return std::views::reverse(std::forward<R>(r));
        }
    };

    template <std::integral T>
    struct drop {
        constexpr explicit drop(T n)
            : n(n) {}

        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const drop &self) {
            return std::views::drop(std::forward<R>(r), self.n);
        }

    private:
        T n;
    };

    template <std::integral T>
    struct take {
        constexpr explicit take(T n)
            : n(n) {}

        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const take &self) {
            return std::views::take(std::forward<R>(r), self.n);
        }

    private:
        T n;
    };

    struct step_by {
        constexpr explicit step_by(int step)
            : step(step) {}

        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const step_by &self) {
            return std::views::stride(std::forward<R>(r), self.step);
        }

    private:
        int step;
    };


    template <std::integral T>
    struct range : std::ranges::iota_view<T, T> {
        constexpr range(T start, T stop)
            : std::ranges::iota_view<T, T>{start, stop} {}

        constexpr explicit range(T stop)
            : range(T(0), stop) {}
    };


    template <std::integral T>
    struct enumerate {
        constexpr enumerate(T start, T stop)
            : r(start, stop) {}

        constexpr explicit enumerate(T start)
            : r(start, std::numeric_limits<T>::max()) {}

        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const enumerate &self) {
            return std::views::zip(self.r, std::forward<R>(r));
        }

    private:
        range<T> r;
    };


    template <typename F>
    struct map {
        constexpr map(F fn)
            : fn(std::move(fn)) {}

        template <std::ranges::range R>
            requires(not tuple_like<std::ranges::range_value_t<R>>)
        friend constexpr auto operator|(R &&r, const map &self) {
            return std::views::transform(std::forward<R>(r), self.fn);
        }

        template <std::ranges::range R>
            requires tuple_like<std::ranges::range_value_t<R>>
        friend constexpr auto operator|(R &&r, const map &self) {
            return std::views::transform(
                std::forward<R>(r),
                [&self](std::ranges::range_value_t<std::remove_reference_t<R>> tuple) { return std::apply(self.fn, tuple); });
        }

        template <typename T>
        friend constexpr std::optional<std::invoke_result_t<F, T>> operator|(const std::optional<T> &opt, const map &self) {
            if (opt.has_value()) {
                return self.fn(opt.value());
            } else {
                return std::nullopt;
            }
        }

        template <typename T>
        friend constexpr std::optional<std::invoke_result_t<F, T>> operator|(std::optional<T> &&opt, const map &self) {
            if (opt.has_value()) {
                return self.fn(std::move(opt.value()));
            } else {
                return std::nullopt;
            }
        }

        template <typename T, typename E>
        friend constexpr std::expected<std::invoke_result_t<F, T>, E> operator|(const std::expected<T, E> &res, const map &self) {
            if (res.has_value()) {
                return self.fn(res.value());
            } else {
                return std::unexpected<E>(res.error());
            }
        }

        template <typename T, typename E>
        friend constexpr std::expected<std::invoke_result_t<F, T>, E> operator|(std::expected<T, E> &&res, const map &self) {
            if (res.has_value()) {
                return self.fn(std::move(res.value()));
            } else {
                return std::unexpected<E>(std::move(res.error()));
            }
        }

        F fn;
    };

    template <typename F>
    struct filter {
        constexpr filter(F fn)
            : fn(std::move(fn)) {}

        template <std::ranges::range R>
            requires(not tuple_like<std::ranges::range_value_t<std::remove_reference_t<R>>>)
        friend constexpr auto operator|(R &&r, const filter &self) {
            return std::views::filter(std::forward<R>(r), self.fn);
        }

        template <std::ranges::range R>
            requires tuple_like<std::ranges::range_value_t<std::remove_reference_t<R>>>
        friend constexpr auto operator|(R &&r, const filter &self) {
            return std::views::filter(std::forward<R>(r), [&self](std::ranges::range_value_t<std::remove_reference_t<R>> tuple) {
                return std::apply(self.fn, tuple);
            });
        }

    private:
        F fn;
    };

    template <typename F>
    struct map_filter {
        constexpr map_filter(F fn)
            : fn(std::move(fn)) {}

        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const map_filter &self) {
            return std::forward<R>(r) | map(self.fn) | filter([](const auto &item) { return bool(item); })
                | map([](auto &&item) { return std::move(*item); });
        }

    private:
        F fn;
    };

    struct join {
        template <std::ranges::range R>
        friend constexpr auto operator|(R &&r, const join &) {
            return std::views::join(std::forward<R>(r));
        }
    };

    template <template <typename...> class Container>
    struct collect {
        template <std::ranges::range R>
            requires (not tuple_like<std::ranges::range_value_t<std::remove_reference_t<R>>>)
        friend constexpr auto operator|(R &&r, const collect &) {
            return Container<std::ranges::range_value_t<std::remove_reference_t<R>>>(
                std::ranges::begin(r), std::ranges::end(r));
        }

        template <std::ranges::range R>
            requires tuple_like<std::ranges::range_value_t<std::remove_reference_t<R>>>
        friend constexpr auto operator|(R &&r, const collect &) {
            return expand_tuple_element_t<std::ranges::range_value_t<std::remove_reference_t<R>>, Container>(
                std::ranges::begin(r), std::ranges::end(r));
        }
    };

} // namespace cppxx

#endif
