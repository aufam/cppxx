#include <fmt/ranges.h>
#include <regex>
#include <filesystem>
#include <cppxx/debug.h>
#include "workspace.h"

namespace fs = std::filesystem;


auto Workspace::expand_variables(const std::string &input) const -> std::string {
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
        if (auto var_it = vars.find(var_name); var_it != vars.end()) {
            value = var_it->second;
        } else if (const char *env = std::getenv(var_name.c_str())) {
            value = env;
        } else if (!fallback.empty()) {
            value = fallback;
        } else {
            throw std::runtime_error(fmt::format("variable `${{{}}}` is not set and no fallback provided\n", var_name));
        }

        result += value;
        last_pos = match.position() + match.length();
    }

    // Append the rest of the string
    result.append(input, last_pos, std::string::npos);
    return result;
}


auto Workspace::expand_path(const std::string &pattern) -> std::vector<std::string> {
    std::vector<std::string> result;

    bool recursive = pattern.find("**") != std::string::npos;
    auto last_slash = pattern.rfind('/');
    std::string dir = last_slash != std::string::npos ? pattern.substr(0, last_slash) : ".";
    std::string filename_pattern = pattern.substr(last_slash + 1);

    // Translate glob to regex:
    // - **  -> .*
    // - *   -> [^/]*   (matches any except slash)
    // - ?   -> .       (any single character)
    std::string regex_str;
    for (size_t i = 0; i < filename_pattern.size(); ++i) {
        if (filename_pattern[i] == '*') {
            if (i + 1 < filename_pattern.size() && filename_pattern[i + 1] == '*') {
                regex_str += ".*";
                ++i; // skip next '*'
            } else {
                regex_str += "[^/]*";
            }
        } else if (filename_pattern[i] == '?') {
            regex_str += '.';
        } else if (std::strchr(".^$|()[]{}+\\", filename_pattern[i])) {
            regex_str += '\\';
            regex_str += filename_pattern[i];
        } else {
            regex_str += filename_pattern[i];
        }
    }

    std::regex regex_pattern("^" + regex_str + "$");

    if (recursive) {
        for (const auto &entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), regex_pattern)) {
                result.push_back(entry.path().string());
            }
        }
    } else {
        for (const auto &entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), regex_pattern)) {
                result.push_back(entry.path().string());
            }
        }
    }

    return result;
}
