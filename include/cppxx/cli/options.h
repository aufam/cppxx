#ifndef CPPXX_CLI_OPTION_H
#define CPPXX_CLI_OPTION_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_set>
#include <exception>


namespace cppxx::cli {
    using Generic = std::variant<bool *,
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
                                 std::optional<std::vector<std::string>> *>;

    struct Option {
        Generic target;
        char key_char = '\0';
        std::string key_str, help;
        bool is_positional = false;
        std::unordered_set<std::variant<int, std::string>> one_of = {};
    };

    inline std::vector<std::string> parse(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);
    inline std::vector<std::string>
    parse_or_throw(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);

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
#if defined(CXXOPTS_HPP_INCLUDED) && defined(FMT_RANGES_H_)
namespace cppxx::cli::detail {
    template <typename T>
    struct is_optional : std::false_type {};

    template <typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    inline std::pair<std::unordered_set<int>, std::unordered_set<std::string>> get_one_of_sets(const cppxx::cli::Option &opt) {
        std::unordered_set<int> one_of_int;
        std::unordered_set<std::string> one_of_str;
        for (const auto &one : opt.one_of) {
            if (std::holds_alternative<int>(one)) {
                one_of_int.emplace(std::get<int>(one));
            } else if (std::holds_alternative<std::string>(one)) {
                one_of_str.emplace(std::get<std::string>(one));
            }
        }

        return {std::move(one_of_int), std::move(one_of_str)};
    }

    template <typename T>
    void visitor_setup(const T *target, cxxopts::OptionAdder &add, const cppxx::cli::Option &opt) {
        std::shared_ptr<cxxopts::Value> val;
        std::string help = opt.help;
        if constexpr (is_optional<T>::value) {
            val = cxxopts::value<typename T::value_type>();
            if (target->has_value())
                help = fmt::format("{}{}(default: {})", help, help.empty() ? "" : " ", target->value());
        } else {
            val = cxxopts::value<T>();
            help = fmt::format("{}{}(required)", help, help.empty() ? "" : " ");
        }

        if (not opt.one_of.empty()) {
            auto [one_of_int, one_of_str] = get_one_of_sets(opt);
            help = fmt::format("{}{}(value: {{{}{:?}}})", help, help.empty() ? "" : " ", fmt::join(one_of_int, ", "),
                               fmt::join(one_of_str, ", "));
        }

        std::string key = opt.key_char == '\0' ? opt.key_str : fmt::format("{},{}", opt.key_char, opt.key_str);
        add(key, help, val);
    }

    template <typename T>
    void visitor_target(T *target, const cxxopts::ParseResult &parser, const cppxx::cli::Option &opt) {
        auto [one_of_int, one_of_str] = get_one_of_sets(opt);

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

inline std::vector<std::string>
cppxx::cli::parse(const std::string &app_name, int argc, char **argv, const std::vector<Option> &opts) {
    try {
        return parse_or_throw(app_name, argc, argv, opts);
    } catch (const parse_help &e) {
        fmt::println(stderr, "{}", e.what());
        exit(0);
    } catch (const parse_error &e) {
        fmt::println(stderr, "{}", e.what());
        exit(1);
    }
}

inline std::vector<std::string>
cppxx::cli::parse_or_throw(const std::string &app_name, int argc, char **argv, const std::vector<Option> &opts) {
    cxxopts::Options options(argv[0], app_name);

    auto add = options.add_options();
    std::vector<std::string> positionals;
    for (const auto &opt : opts) {
        std::visit([&](auto *target) { detail::visitor_setup(target, add, opt); }, opt.target);
        if (opt.is_positional)
            positionals.push_back(opt.key_str);
    }
    add("h,help", "Print help");

    if (not positionals.empty()) {
        options.parse_positional(positionals);
        options.positional_help(fmt::format("<{}>", fmt::join(positionals, "> <")));
        options.show_positional_help();
    }

    try {
        auto parser = options.parse(argc, argv);
        if (parser.count("help"))
            throw parse_help(options.help());

        for (const auto &opt : opts)
            std::visit([&](auto *target) { detail::visitor_target(target, parser, opt); }, opt.target);
        return parser.unmatched();
    } catch (const cxxopts::exceptions::exception &e) {
        throw parse_error(fmt::format("Failed to parse options: {}", e.what()));
    }
}

#    if defined(RFL_RFL_HPP_)
namespace cppxx::cli::detail {

    struct Context {
        std::unordered_map<std::string_view, bool> bools;
        std::unordered_map<std::string_view, int> ints;
        std::unordered_map<std::string_view, float> floats;
        std::unordered_map<std::string_view, std::string> strings;

        std::unordered_map<std::string_view, std::optional<int>> opt_int;
        std::unordered_map<std::string_view, std::optional<float>> opt_float;
        std::unordered_map<std::string_view, std::optional<std::string>> opt_string;

        std::unordered_map<std::string_view, std::vector<int>> arr_int;
        std::unordered_map<std::string_view, std::vector<float>> arr_float;
        std::unordered_map<std::string_view, std::vector<std::string>> arr_string;

        std::unordered_map<std::string_view, std::optional<std::vector<int>>> opt_arr_int;
        std::unordered_map<std::string_view, std::optional<std::vector<float>>> opt_arr_float;
        std::unordered_map<std::string_view, std::optional<std::vector<std::string>>> opt_arr_string;

        template <typename T>
        T &set(std::string_view key, T value) {
            auto &map = getMap<T>();
            return map[key] = std::move(value);
        }

        template <typename T>
        T &get(std::string_view key) {
            auto &map = getMap<T>();
            return map.at(key);
        }

    private:
        template <typename T>
        auto &getMap() {
            if constexpr (std::is_same_v<T, bool>)
                return bools;
            else if constexpr (std::is_same_v<T, int>)
                return ints;
            else if constexpr (std::is_same_v<T, float>)
                return floats;
            else if constexpr (std::is_same_v<T, std::string>)
                return strings;
            else if constexpr (std::is_same_v<T, std::optional<int>>)
                return opt_int;
            else if constexpr (std::is_same_v<T, std::optional<float>>)
                return opt_float;
            else if constexpr (std::is_same_v<T, std::optional<std::string>>)
                return opt_string;
            else if constexpr (std::is_same_v<T, std::vector<int>>)
                return arr_int;
            else if constexpr (std::is_same_v<T, std::vector<float>>)
                return arr_float;
            else if constexpr (std::is_same_v<T, std::vector<std::string>>)
                return arr_string;
            else if constexpr (std::is_same_v<T, std::optional<std::vector<int>>>)
                return opt_arr_int;
            else if constexpr (std::is_same_v<T, std::optional<std::vector<float>>>)
                return opt_arr_float;
            else if constexpr (std::is_same_v<T, std::optional<std::vector<std::string>>>)
                return opt_arr_string;
            else
                return ints;
        }
    };

    constexpr std::array<std::string_view, 8> split(std::string_view s, char delim) {
        std::array<std::string_view, 8> parts{};

        for (size_t count = 0; !s.empty() && count < parts.size(); ++count) {
            std::size_t pos = s.find(delim);
            if (pos == std::string_view::npos) {
                parts[count] = s;
                break;
            }
            parts[count] = s.substr(0, pos);
            s.remove_prefix(pos + 1);
        }

        return parts;
    }

    template <typename T>
    cppxx::cli::Option make_option_from_field(Context &ctx, std::string_view spec, T &field) {
        cppxx::cli::Option opt{};
        if constexpr (std::is_enum_v<T>) { // TODO: std::optional<Enum>?
            opt.target = &ctx.set(spec, rfl::enum_to_string(field));
            rfl::get_enumerators<T>().apply([&](const auto &e) { opt.one_of.emplace(std::string(e.name())); });
        } else {
            opt.target = &ctx.set(spec, field);
        }

        for (auto &p : split(spec, ',')) {
            if (p.empty())
                continue;

            if (p.size() == 1 && std::isalpha(static_cast<unsigned char>(p[0]))) {
                opt.key_char = p[0];
            } else if (p.starts_with("help=")) {
                opt.help = p.substr(5);
            } else if (p == "positional") {
                opt.is_positional = true;
            } else {
                opt.key_str = p;
            }
        }

        return opt;
    }

    template <typename T>
    void assign_field_from_context(Context &ctx, std::string_view spec, T &field) {
        if constexpr (std::is_enum_v<T>)
            field = rfl::string_to_enum<T>(ctx.get<std::string>(spec)).value();
        else
            field = ctx.get<T>(spec);
    }
} // namespace cppxx::cli::detail

namespace cppxx::cli {
    template <rfl::internal::StringLiteral name, typename T>
    using Tag = rfl::Rename<name, T>;

    /// T must be a user-defined struct where all fields are tagged using `cppxx::cli::Tag`
    /// @example
    /// ```c++
    // struct User {
    //     cppxx::cli::Tag<"name,positional,help=Specify user's name", std::string> name; // positional argument
    //     cppxx::cli::Tag<"a,age,help=specify user's age", int> age; // required option with flag -a,--age
    //     cppxx::cli::Tag<"email", std::optional<std::string>> email = "user@example.com"; // optional arg with default value
    // };
    /// ```
    template <typename T>
    std::vector<std::string> parse_tagged(const std::string &app_name, int argc, char **argv, T &options) {
        detail::Context ctx = {};

        std::vector<cppxx::cli::Option> opts;
        auto view = rfl::to_view(options);
        view.apply([&](auto &&f) { opts.push_back(detail::make_option_from_field(ctx, f.name(), f.value()->value())); });

        auto rest = cppxx::cli::parse(app_name, argc, argv, opts);
        view.apply([&](auto &f) { detail::assign_field_from_context(ctx, f.name(), f.value()->value()); });

        return rest;
    }
} // namespace cppxx::cli
#    endif
#endif
#endif
