#ifndef CPPXX_DEBUG_H
#define CPPXX_DEBUG_H

#include <string>

namespace cppxx {
    void info(const char *file, int line, const std::string &msg);
    void warning(const char *file, int line, const std::string &msg);
    void panic(const char *file, int line, const std::string &msg);
} // namespace cppxx


#ifdef FMT_FORMAT_H_
#    define INFO(...)    ::cppxx::info(__FILE__, __LINE__, fmt::format(__VA_ARGS__));
#    define WARNING(...) ::cppxx::warning(__FILE__, __LINE__, fmt::format(__VA_ARGS__));
#    define PANIC(...)   ::cppxx::panic(__FILE__, __LINE__, fmt::format(__VA_ARGS__));
#    define DBG(...)                                                                             \
        [&]() -> decltype(auto) {                                                                \
            if constexpr (std::is_void<decltype(__VA_ARGS__)>::value) {                          \
                ::cppxx::info(__FILE__, __LINE__, fmt::format("{} = void", #__VA_ARGS__));       \
                return;                                                                          \
            } else if constexpr (std::is_lvalue_reference<decltype(__VA_ARGS__)>::value) {       \
                decltype(auto) _value = __VA_ARGS__;                                             \
                ::cppxx::info(__FILE__, __LINE__, fmt::format("{} = {}", #__VA_ARGS__, _value)); \
                return _value;                                                                   \
            } else {                                                                             \
                auto _value = __VA_ARGS__;                                                       \
                ::cppxx::info(__FILE__, __LINE__, fmt::format("{} = {}", #__VA_ARGS__, _value)); \
                return _value;                                                                   \
            }                                                                                    \
        }()
#else
#    define INFO(msg)    ::cppxx::info(__FILE__, __LINE__, msg);
#    define WARNING(msg) ::cppxx::warning(__FILE__, __LINE__, msg);
#    define PANIC(msg)   ::cppxx::panic(__FILE__, __LINE__, msg);
#endif
#endif
