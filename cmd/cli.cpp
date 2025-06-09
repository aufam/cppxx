#include <cxxopts.hpp>
#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include "cli.h"


#define DEFINE_OPTION_FOR(T)                                                                                        \
    template <>                                                                                                     \
    auto Option::add_option(                                                                                        \
        T &target, std::string key, std::string help, std::optional<std::string> default_value, bool is_positional) \
        -> Option & {                                                                                               \
        caches.push_back(Cache{&target, '\0', key, help, default_value, is_positional});                            \
        return *this;                                                                                               \
    }                                                                                                               \
    template <>                                                                                                     \
    auto Option::add_option(T &target,                                                                              \
                            char key_char,                                                                          \
                            std::string key_str,                                                                    \
                            std::string help,                                                                       \
                            std::optional<std::string> default_value,                                               \
                            bool is_positional) -> Option & {                                                       \
        caches.push_back(Cache{&target, key_char, key_str, help, default_value, is_positional});                    \
        return *this;                                                                                               \
    }

DEFINE_OPTION_FOR(int)
DEFINE_OPTION_FOR(float)
DEFINE_OPTION_FOR(bool)
DEFINE_OPTION_FOR(std::string)
DEFINE_OPTION_FOR(std::vector<int>)
DEFINE_OPTION_FOR(std::vector<float>)
DEFINE_OPTION_FOR(std::vector<bool>)
DEFINE_OPTION_FOR(std::vector<std::string>)

std::string Option::parse(const std::string &app_name, int argc, char **argv) {
    cxxopts::Options options(argv[0], app_name);
    std::vector<std::string> positionals;
    std::string positional_help;

    for (const auto &cache : caches) {
        std::visit(
            [&](auto *target) {
                auto val = cxxopts::value<std::remove_pointer_t<decltype(target)>>();
                if (cache.default_value)
                    val = val->default_value(*cache.default_value);

                std::string key = cache.key_char == '\0' ? cache.key_str : fmt::format("{},{}", cache.key_char, cache.key_str);
                options.add_options()(cache.key_str, cache.help, val);
            },
            cache.target);

        if (cache.is_positional) {
            positionals.push_back(cache.key_str);
            positional_help += " [" + cache.key_str + "]";
        }
    }

    options.add_options()("json", "JSON input", cxxopts::value<std::string>()->default_value("{}"))("h,help", "Print help");

    if (!positionals.empty()) {
        options.parse_positional(positionals);
        options.positional_help(positional_help);
        options.show_positional_help();
    }

    nlohmann::json j;
    try {
        auto parser = options.parse(argc, argv);
        if (parser.count("help")) {
            fmt::println("{}", options.help());
            exit(0);
        }

        j = nlohmann::json::parse(parser["json"].as<std::string>());

        for (const auto &cache : caches) {
            std::visit(
                [&](auto *target) {
                    if (j.count(cache.key_str)) {
                        j.at(cache.key_str).get_to(*target);
                    } else {
                        *target = parser[cache.key_str].as<std::remove_pointer_t<decltype(target)>>();
                        j[cache.key_str] = *target;
                    }
                },
                cache.target);
        }
    } catch (const cxxopts::exceptions::exception &e) {
        fmt::println(stderr, "Failed to parse options: {}", e.what());
        exit(1);
    } catch (const nlohmann::json::exception &e) {
        fmt::println(stderr, "Failed to parse json: {}", e.what());
        exit(1);
    }

    return j.dump(2);
}
