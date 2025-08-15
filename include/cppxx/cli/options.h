#ifndef CPPXX_CLI_OPTION_H
#define CPPXX_CLI_OPTION_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_set>
#include <exception>


namespace cppxx::cli {
    struct Option {
        std::variant<bool *,
                     int *,
                     float *,
                     std::string *,

                     std::optional<int> *,
                     std::optional<float> *,
                     std::optional<std::string> *,

                     std::vector<int> *,
                     std::vector<float> *,
                     std::vector<std::string> *,

                     std::optional<std::vector<int>> *,
                     std::optional<std::vector<float>> *,
                     std::optional<std::vector<std::string>> *>
            target;
        char key_char = '\0';
        std::string key_str, help;
        bool is_positional = false;
        std::unordered_set<std::variant<int, std::string>> one_of = {};
    };

    inline void parse(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);
    inline void parse_or_throw(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);

    class parse_error : public std::exception {
    public:
        explicit parse_error(std::string msg)
            : msg(std::move(msg)) {}
        ~parse_error() override = default;

        [[nodiscard]] const char *what() const noexcept override { return msg.c_str(); }

    private:
        std::string msg;
    };

    class parse_help : public std::exception {
    public:
        explicit parse_help(std::string msg)
            : msg(std::move(msg)) {}
        ~parse_help() override = default;

        [[nodiscard]] const char *what() const noexcept override { return msg.c_str(); }

    private:
        std::string msg;
    };
} // namespace cppxx::cli

// jarro2783/cxxopts interopt
#ifdef CXXOPTS_HPP_INCLUDED
namespace cppxx::cli::detail {
    template <typename T>
    struct is_optional : std::false_type {};

    template <typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    template <typename T>
    void visitor_setup(const T *target, cxxopts::OptionAdder &add, const cppxx::cli::Option &opt) {
        std::shared_ptr<cxxopts::Value> val;
        std::string help = opt.help;
        if constexpr (is_optional<T>::value) {
            val = cxxopts::value<typename T::value_type>();
            if (not help.empty()) {
                if (target->has_value()) {
                    help = fmt::format("{} (default: {})", help, target->value());
                } else {
                    help = fmt::format("{} (default: <not set>)", help);
                }
            }
        } else {
            val = cxxopts::value<T>();
        }
        std::string key = opt.key_char == '\0' ? opt.key_str : fmt::format("{},{}", opt.key_char, opt.key_str);
        add(key, help, val);
    }

    template <typename T>
    void visitor_target(T *target, const cxxopts::ParseResult &parser, const cppxx::cli::Option &opt) {
        std::unordered_set<int> one_of_int;
        std::unordered_set<std::string> one_of_str;
        for (const auto &one : opt.one_of) {
            if (std::holds_alternative<int>(one)) {
                one_of_int.emplace(std::get<int>(one));
            } else if (std::holds_alternative<std::string>(one)) {
                one_of_str.emplace(std::get<std::string>(one));
            }
        }

        if constexpr (is_optional<T>::value) {
            if (parser.count(opt.key_str)) {
                *target = parser[opt.key_str].as<typename T::value_type>();
                if constexpr (std::is_same_v<typename T::value_type, int>) {
                    if (not opt.one_of.empty() and one_of_int.count(**target) == 0) {
                        throw cppxx::cli::parse_error(fmt::format("Assertion error: {} not in {}", **target, one_of_int));
                    }
                } else if constexpr (std::is_same_v<typename T::value_type, std::string>) {
                    if (not opt.one_of.empty() and one_of_str.count(**target) == 0) {
                        throw cppxx::cli::parse_error(fmt::format("Assertion error: {:?} not in {}", **target, one_of_str));
                    }
                }
            }
        } else {
            *target = parser[opt.key_str].as<T>();
            if constexpr (std::is_same_v<T, int>) {
                if (not opt.one_of.empty() and one_of_int.count(*target) == 0) {
                    throw cppxx::cli::parse_error(fmt::format("Assertion error: {} not in {}", *target, one_of_int));
                }
            } else if constexpr (std::is_same_v<T, std::string>) {
                if (not opt.one_of.empty() and one_of_str.count(*target) == 0) {
                    throw cppxx::cli::parse_error(fmt::format("Assertion error: {:?} not in {}", *target, one_of_str));
                }
            }
        }
    }
} // namespace cppxx::cli::detail

inline void cppxx::cli::parse(const std::string &app_name, int argc, char **argv, const std::vector<Option> &opts) {
    try {
        parse_or_throw(app_name, argc, argv, opts);
    } catch (const parse_help &e) {
        fmt::println(stderr, "{}", e.what());
        exit(0);
    } catch (const parse_error &e) {
        fmt::println(stderr, "{}", e.what());
        exit(1);
    }
}

inline void cppxx::cli::parse_or_throw(const std::string &app_name, int argc, char **argv, const std::vector<Option> &opts) {
    std::vector<std::string> positionals;
    std::string positional_help;

    cxxopts::Options options(argv[0], app_name);
    auto add = options.add_options();
    for (const auto &opt : opts) {
        std::visit([&](auto *target) { detail::visitor_setup(target, add, opt); }, opt.target);

        if (opt.is_positional) {
            positionals.push_back(opt.key_str);
            positional_help += " [" + opt.key_str + "]";
        }
    }
    add("h,help", "Print help");

    if (!positionals.empty()) {
        options.parse_positional(positionals);
        options.positional_help(positional_help);
        options.show_positional_help();
    }

    try {
        auto parser = options.parse(argc, argv);
        if (parser.count("help"))
            throw parse_help(options.help());

        for (const auto &opt : opts)
            std::visit([&](auto *target) { detail::visitor_target(target, parser, opt); }, opt.target);
    } catch (const cxxopts::exceptions::exception &e) {
        throw parse_error(fmt::format("Failed to parse options: {}", e.what()));
    }
}
#endif
#endif
