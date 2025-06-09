#include <fmt/format.h>
#include <fmt/color.h>
#include <cppxx/debug.h>
#include <stdexcept>

#ifdef _WIN32
#    define WEAK
#else
#    define WEAK __attribute__((weak))
#endif

WEAK void cppxx::info(const char *file, int line, const std::string &msg) {
    fmt::print(stderr, "[{}:{}] ", file, line);
    fmt::print(stderr, fmt::fg(fmt::terminal_color::green), "INFO");
    fmt::println(stderr, " {}", msg);
}

WEAK void cppxx::warning(const char *file, int line, const std::string &msg) {
    fmt::print(stderr, "[{}:{}] ", file, line);
    fmt::print(stderr, fmt::fg(fmt::terminal_color::yellow), "WARNING");
    fmt::println(stderr, " {}", msg);
}

WEAK void cppxx::panic(const char *file, int line, const std::string &msg) {
    throw std::runtime_error(fmt::format("panic at {}:{}: {}\n", file, line, msg));
}
