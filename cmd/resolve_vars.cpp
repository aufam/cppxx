#include <fmt/ranges.h>
#include <cppxx/iterator.h>
#include <cppxx/error.h>
#include <regex>
#include "workspace.h"


template <typename T>
T expand_variables(const std::unordered_map<std::string, std::string> &vars, const T &input, bool only_env_vars = false);

template <>
std::string
expand_variables(const std::unordered_map<std::string, std::string> &vars, const std::string &input, bool only_env_vars) {
    const std::regex pattern(R"(\$\{([A-Za-z_][A-Za-z0-9_]*)(:-([^}]*))?\})");
    std::string result;
    std::sregex_iterator begin(input.begin(), input.end(), pattern);
    std::sregex_iterator end;

    std::size_t last_pos = 0;
    for (auto it = begin; it != end; ++it) {
        const std::smatch &match = *it;

        // Append text before match
        result.append(input, last_pos, match.position() - last_pos);

        std::string var_name = match[1].str();
        std::string fallback = match[3].matched ? match[3].str() : "";

        std::string value;
        if (auto var_it = vars.find(var_name); not only_env_vars and var_it != vars.end()) {
            value = var_it->second;
        } else if (const char *env = std::getenv(var_name.c_str())) {
            value = env;
        } else if (!fallback.empty()) {
            value = fallback;
        } else {
            throw std::runtime_error(
                fmt::format("Failed to resolved {:?}: variable `${{{}}}` is not set and no fallback provided. vars = {}", input,
                            var_name, vars));
        }

        result += value;
        last_pos = match.position() + match.length();
    }

    // Append the rest of the string
    result.append(input, last_pos, std::string::npos);
    return result;
}


template <>
std::vector<std::string> expand_variables(const std::unordered_map<std::string, std::string> &vars,
                                          const std::vector<std::string> &inputs,
                                          bool only_env_vars) {
    return inputs | cppxx::map([&](const std::string &input) { return expand_variables(vars, input, only_env_vars); })
        | cppxx::collect<std::vector>();
}


template <>
Git expand_variables(const std::unordered_map<std::string, std::string> &vars, const Git &input, bool only_env_vars) {
    return {
        .url = expand_variables(vars, input.url, only_env_vars),
        .tag = expand_variables(vars, input.tag, only_env_vars),
    };
}

template <>
std::variant<Extended, std::vector<std::string>> expand_variables(const std::unordered_map<std::string, std::string> &vars,
                                                                  const std::variant<Extended, std::vector<std::string>> &input,
                                                                  bool only_env_vars) {
    if (std::holds_alternative<Extended>(input)) {
        auto &e = std::get<Extended>(input);
        return Extended{
            .public_ = expand_variables(vars, e.public_(), only_env_vars),
            .private_ = expand_variables(vars, e.private_(), only_env_vars),
        };
    } else if (std::holds_alternative<std::vector<std::string>>(input)) {
        return expand_variables(vars, std::get<std::vector<std::string>>(input), only_env_vars);
    } else {
        return {};
    }
}


template <typename T>
std::optional<T>
expand_variables(const std::unordered_map<std::string, std::string> &vars, const std::optional<T> &input, bool only_env_vars) {
    if (not input)
        return std::nullopt;
    return expand_variables(vars, input.value(), only_env_vars);
}


template <>
Target expand_variables(const std::unordered_map<std::string, std::string> &vars, const Target &t, bool only_env_vars) {
    return {
        .archive = expand_variables(vars, t.archive, only_env_vars),
        .git = expand_variables(vars, t.git, only_env_vars),
        .sources = expand_variables(vars, t.sources, only_env_vars),
        .include_dirs = expand_variables(vars, t.include_dirs, only_env_vars),
        .flags = expand_variables(vars, t.flags, only_env_vars),
        .link_flags = expand_variables(vars, t.link_flags, only_env_vars),
        .depends_on = expand_variables(vars, t.depends_on, only_env_vars),
    };
}


std::expected<Workspace, std::runtime_error> resolve_vars(Workspace &&w) {
    if (not w.vars)
        return w;

    auto &vars = w.vars.value();
    try {
        for (auto &[_, v] : vars)
            v = expand_variables(vars, v, true);

        w.title = expand_variables(vars, w.title);
        w.version = expand_variables(vars, w.version);
        w.compiler = expand_variables(vars, w.compiler);
        w.author = expand_variables(vars, w.author);

        if (w.interface)
            for (auto &[_, t] : w.interface.value())
                t = expand_variables(vars, t);

        if (w.lib)
            for (auto &[_, t] : w.lib.value())
                t = expand_variables(vars, t);

        if (w.bin)
            for (auto &[_, t] : w.bin.value())
                t = expand_variables(vars, t);
    } catch (std::runtime_error &e) {
        return cppxx::unexpected_errorf("Failed to resolve paths: {}", e.what());
    }

    return w;
}
