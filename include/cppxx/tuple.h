#ifndef CPPXX_TUPLE_H
#define CPPXX_TUPLE_H

#include <tuple>
#include <type_traits>
#include <utility>


namespace cppxx {

    template <typename T>
    concept tuple_like = requires { typename std::tuple_size<T>::type; };

    template <tuple_like T, template <typename...> class R>
    struct expand_tuple_element {
    private:
        template <std::size_t... N>
        static auto impl(std::index_sequence<N...>) -> R<std::decay_t<std::tuple_element_t<N, T>>...>;
        static constexpr auto seq = std::make_index_sequence<std::tuple_size_v<T>>();

    public:
        using type = decltype(impl(seq));
    };

    template <tuple_like T, template <typename...> class R>
    using expand_tuple_element_t = typename expand_tuple_element<T, R>::type;

} // namespace cppxx

#endif
