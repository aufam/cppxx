#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <filesystem>
#include <regex>
#include "workspace.h"

namespace fs = std::filesystem;


static auto expand_path(const std::string &pattern) -> std::vector<std::string> {
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

static void collect_expanded_paths(const std::string &root,
                                   const std::unordered_map<std::string, std::string> &populated,
                                   std::vector<std::string> &paths,
                                   bool expand = true) {
    std::vector<std::string> collected;
    for (auto src : paths) {
        src = populated.at(src);
        if (fs::path(src).is_relative())
            src = root / fs::path(src);

        std::vector<std::string> to_append = expand ? expand_path(src) : std::vector{src};
        std::ranges::move(to_append, std::back_inserter(collected));
    }

    paths = std::move(collected);
}

void resolve_paths(Target &t, const std::unordered_map<std::string, std::string> &populated) {
    fs::path root;
    if (t.git) {
        root = populated.at(t.git->as_key());
    } else if (t.archive) {
        root = populated.at(t.archive.value());
    } else {
        root = fs::current_path();
    }

    if (t.sources)
        collect_expanded_paths(root, populated, t.sources.value());

    if (t.include_dirs && std::holds_alternative<Extended>(t.include_dirs.value())) {
        collect_expanded_paths(root, populated, std::get<Extended>(t.include_dirs.value()).public_(), false);
        collect_expanded_paths(root, populated, std::get<Extended>(t.include_dirs.value()).private_(), false);
    } else if (t.include_dirs && std::holds_alternative<std::vector<std::string>>(t.include_dirs.value())) {
        collect_expanded_paths(root, populated, std::get<std::vector<std::string>>(t.include_dirs.value()), false);
    }
}

std::expected<Workspace, std::runtime_error> resolve_paths(Workspace &&w) {
    auto &populated = w.populated();
    try {
        if (w.interface)
            for (auto &[name, t] : w.interface.value())
                resolve_paths(t, populated);
        if (w.lib)
            for (auto &[name, t] : w.lib.value())
                resolve_paths(t, populated);
        if (w.bin)
            for (auto &[name, t] : w.bin.value())
                resolve_paths(t, populated);
    } catch (std::runtime_error &e) {
        return cppxx::unexpected_errorf("Failed to resolve paths: {}", e.what());
    }

    return std::move(w);
}
