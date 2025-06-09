#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <toml.hpp>
#include <openssl/sha.h>
#include <cppxx/debug.h>
#include <regex>
#include <filesystem>
#include <cstdlib>
#include <unordered_set>
#include "common.h"

namespace fs = std::filesystem;


auto expand_variables(const std::string &input, const std::unordered_map<std::string, std::string> &vars) -> std::string {
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
            WARNING("variable `${}` is not set and no fallback provided\n", var_name);
        }

        result += value;
        last_pos = match.position() + match.length();
    }

    // Append the rest of the string
    result.append(input, last_pos, std::string::npos);
    return result;
}


auto expand_path(const std::string &pattern) -> std::vector<std::string> {
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


void assign_string(const toml::node &node,
                   const std::unordered_map<std::string, std::string> &vars,
                   const std::string &key,
                   std::string &value) {
    if (not node.is_string())
        throw std::runtime_error(fmt::format("\"{}\" must be a string", key));

    value = expand_variables(node.value_or(""), vars);
}

void assign_int(const toml::node &node, const std::string &key, int &value) {
    if (not node.is_integer())
        throw std::runtime_error(fmt::format("\"{}\" must be a string", key));

    value = node.value_or(0);
}

void assign_list(const toml::node &node,
                 const std::unordered_map<std::string, std::string> &vars,
                 const std::string &key,
                 std::vector<std::string> &private_values,
                 std::vector<std::string> &public_values,
                 std::vector<std::string> &unknown_values) {
    if (node.is_table()) {
        if (node.as_table()->contains("public"))
            for (auto &src : *node.as_table()->at("public").as_array())
                public_values.push_back(expand_variables(src.value_or(""), vars));

        if (node.as_table()->contains("private"))
            for (auto &src : *node.as_table()->at("private").as_array())
                private_values.push_back(expand_variables(src.value_or(""), vars));

        return;
    }

    if (not node.is_array())
        throw std::runtime_error(fmt::format("\"{}\" must be a list of strings", key));

    for (auto &src : *node.as_array())
        unknown_values.push_back(expand_variables(src.value_or(""), vars));
}

void assign_paths(const toml::node &node,
                  const std::unordered_map<std::string, std::string> &vars,
                  const std::string &key,
                  std::vector<std::string> &paths) {
    if (not node.is_array())
        throw std::runtime_error(fmt::format("\"{}\" must be a list of strings", key));

    for (auto &src : *node.as_array()) {
        auto path_to_append = expand_variables(src.value_or(""), vars);
        paths.push_back(path_to_append);
    }
}


std::string extract_host_and_path(const std::string &url) {
    std::string cleaned = url;

    // Remove trailing ".git" if present
    if (cleaned.ends_with(".git"))
        cleaned = cleaned.substr(0, cleaned.size() - 4);

    // Handle git@github.com:user/repo
    static const std::regex ssh_pattern(R"(git@([^:]+):(.+))");
    std::smatch ssh_match;
    if (std::regex_match(cleaned, ssh_match, ssh_pattern))
        return ssh_match[1].str() + "/" + ssh_match[2].str();

    // Handle https://github.com/user/repo or http://...
    static const std::regex https_pattern(R"(https?://([^/]+)/(.+))");
    std::smatch https_match;
    if (std::regex_match(cleaned, https_match, https_pattern))
        return https_match[1].str() + "/" + https_match[2].str();

    // Handle github.com/user/repo or user/repo
    return cleaned;
}

bool is_full_url(const std::string &url) {
    return url.starts_with("http://") || url.starts_with("https://") || url.starts_with("git@");
}

bool has_host_prefix(const std::string &url) {
    // Check for common domain prefixes (gitlab.com/, bitbucket.org/, etc.)
    return url.find('.') != std::string::npos && url.find('/') != std::string::npos;
}

std::string normalize_git_url(const std::string &url) {
    if (is_full_url(url)) {
        return url;
    }
    if (has_host_prefix(url)) {
        return "https://" + url; // e.g., gitlab.com/user/repo
    }
    // Otherwise assume it's a shorthand like user/repo
    return "https://github.com/" + url;
}

fs::path populate_git(const toml::node &node, const std::unordered_map<std::string, std::string> &vars, const std::string &key) {
    if (not node.is_table() or not node.as_table()->contains("url") or not node.as_table()->contains("tag"))
        throw std::runtime_error(fmt::format("\"{}\" must be a table containing \"url\" and \"tag\"", key));

    std::string url = normalize_git_url(expand_variables(node.as_table()->at("url").value_or(""), vars));
    std::string tag = expand_variables(node.as_table()->at("tag").value_or(""), vars);

    std::string cache_root = std::getenv("CPPXX_CACHE");
    std::string target_path = fmt::format("{}/{}/{}", cache_root, extract_host_and_path(url), tag);

    if (fs::exists(target_path)) {
        fmt::println(stderr, "Git repo {}/{} already cloned at {}", url, tag, target_path);
        return target_path;
    }

    std::string cmd = fmt::format("git clone --depth 1 --branch {} {} {}", tag, url, target_path);
    fmt::println(stderr, "executing `{}`", cmd);
    if (std::system(cmd.c_str()) != 0)
        throw std::runtime_error(fmt::format("Failed to clone repo from {}", url));

    return target_path;
}


void resolve_dependencies(const std::string &name,
                          std::unordered_map<std::string, Project> &projects,
                          std::unordered_set<std::string> &visited,
                          std::vector<std::string> &stack) {
    if (std::find(stack.begin(), stack.end(), name) != stack.end())
        throw std::runtime_error(fmt::format("Circular dependency detected: {}", name));

    if (visited.count(name))
        return;

    stack.push_back(name);
    auto &project = projects.at(name);

    for (const auto &dep : project.private_depends_on) {
        auto it = projects.find(dep);
        if (it == projects.end())
            throw std::runtime_error(fmt::format("\"workspace.project.{}\" is not found", dep));

        resolve_dependencies(dep, projects, visited, stack);
        project.private_flags.insert(project.private_flags.end(), it->second.public_flags.begin(), it->second.public_flags.end());
        project.private_include_dirs.insert(
            project.private_include_dirs.end(), it->second.public_include_dirs.begin(), it->second.public_include_dirs.end());
    }

    for (const auto &dep : project.public_depends_on) {
        auto it = projects.find(dep);
        if (it == projects.end())
            throw std::runtime_error(fmt::format("\"workspace.project.{}\" is not found", dep));

        resolve_dependencies(dep, projects, visited, stack);
        project.public_flags.insert(project.public_flags.end(), it->second.public_flags.begin(), it->second.public_flags.end());
        project.public_include_dirs.insert(
            project.public_include_dirs.end(), it->second.public_include_dirs.begin(), it->second.public_include_dirs.end());
    }

    stack.pop_back();
    visited.insert(name);
}

std::string encrypt(const std::string &input, size_t length = 10) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash);

    std::ostringstream oss;
    for (size_t i = 0; i < length / 2 && i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}


void generate_compile_commands(Workspace &w, const std::string &mode) {
    const std::unordered_map<std::string, std::string> mode_map = {
        {"debug",   "-g -fsanitize=address,undefined"},
        {"release", "-O3 -DNDEBUG"                   }
    };

    if (!mode_map.contains(mode))
        throw std::runtime_error("Unknown build mode");

    for (auto &[project_name, project] : w.project) {
        for (auto &src : project.sources) {
            CompileCommands cc;

            cc.file = project.base_path + "/" + src;
            cc.output = src + ".o";

            std::vector<std::string> flags;
            for (auto dir : project.private_include_dirs)
                flags.push_back("-I" + dir);
            for (auto dir : project.public_include_dirs)
                flags.push_back("-I" + dir);

            for (auto flag : project.private_flags)
                flags.push_back(flag);
            for (auto flag : project.public_flags)
                flags.push_back(flag);

            cc.command = fmt::format("{} -std=c++{} {}", w.compiler, w.standard, fmt::join(flags, " "));

            std::string cppxx_cache = std::getenv("CPPXX_CACHE") ? std::getenv("CPPXX_CACHE") : "./.cppxx_cache";
            if (project.base_path.starts_with(cppxx_cache)) {
                cc.directory = project.base_path + "/.cppxx/" + mode + "/" + encrypt(cc.command);
            } else {
                cc.directory = cppxx_cache + "/build/" + mode + "/" + w.title + "/" + w.version + "/" + encrypt(cc.command);
            }

            cc.command = fmt::format("{} {} -o {} -c {}", cc.command, mode_map.at(mode), cc.output, cc.file);

            project.compile_commands.push_back(cc);
        }
    }
}


auto parse_workspace(std::string root_dir, std::string mode) -> Workspace {
    fs::path cppxx_cache = std::getenv("CPPXX_CACHE");
    if (cppxx_cache.string().empty())
        throw std::runtime_error("CPPXX_CACHE is not specified");

    auto config = toml::parse_file(root_dir + "cppxx.toml");
    auto w = Workspace{};

    if (not config.contains("workspace"))
        throw std::runtime_error("\"workspace\" table is missing");

    if (not config.at("workspace").is_table())
        throw std::runtime_error("\"workspace\" must be a table");

    for (auto &[key, node] : *config.at("workspace").as_table()) {
        if (key == "title") {
            assign_string(node, w.vars, "workspace.title", w.title);
        } else if (key == "version") {
            assign_string(node, w.vars, "workspace.version", w.version);
        } else if (key == "author") {
            assign_string(node, w.vars, "workspace.author", w.author);
        } else if (key == "compiler") {
            assign_string(node, w.vars, "workspace.compiler", w.compiler);
        } else if (key == "standard") {
            assign_int(node, "workspace.standard", w.standard);
        } else {
            WARNING("unused key \"workspace.{}\"", std::string(key));
        }
    }

    if (w.title.empty())
        throw std::runtime_error("\"workspace.title\" must be specified");

    if (w.version.empty())
        throw std::runtime_error("\"workspace.version\" must be specified");

    switch (w.standard) {
    case 11:
    case 14:
    case 17:
    case 20:
    case 23:
    case 26:
        break;
    default:
        throw std::runtime_error(fmt::format("unknown c++ standard \"{}\"", w.standard));
    }

    if (not config.contains("project")) {
        WARNING("no project found");
        return w;
    }

    if (not config.at("project").is_table())
        throw std::runtime_error("\"project\" must be a table");

    for (auto &[name, project] : *config.at("project").as_table()) {
        if (not project.is_table())
            throw std::runtime_error(fmt::format("\"workspace.project.{}\" must be a table", std::string(name)));

        auto proj = Project{};
        std::vector<std::string> flags, include_dirs, depends_on, sources;
        fs::path base_path = std::getenv("PWD");
        if (not root_dir.empty())
            base_path = base_path / root_dir;

        for (auto &[key, node] : *project.as_table()) {
            if (key == "type") {
                assign_string(node, w.vars, fmt::format("workspace.project.{}.type", std::string(name)), proj.type);
            } else if (key == "sources") {
                assign_paths(node, w.vars, fmt::format("workspace.project.{}.sources", std::string(name)), sources);
            } else if (key == "flags") {
                assign_list(node,
                            w.vars,
                            fmt::format("workspace.project.{}.flags", std::string(name)),
                            proj.private_flags,
                            proj.public_flags,
                            flags);
            } else if (key == "include_dirs") {
                assign_list(node,
                            w.vars,
                            fmt::format("workspace.project.{}.include_dirs", std::string(name)),
                            proj.private_include_dirs,
                            proj.public_include_dirs,
                            include_dirs);
            } else if (key == "depends_on") {
                assign_list(node,
                            w.vars,
                            fmt::format("workspace.project.{}.depends_on", std::string(name)),
                            proj.private_depends_on,
                            proj.public_depends_on,
                            depends_on);
            } else if (key == "git") {
                base_path = populate_git(node, w.vars, fmt::format("workspace.project.{}.git", std::string(name)));
            } else {
                WARNING("unused key \"workspace.project.{}.{}\"", std::string(name), std::string(key));
            }
        }

        // Normalize type
        if (proj.type != "interface" and proj.type != "executable" and proj.type != "static" and proj.type != "dynamic")
            throw std::runtime_error(fmt::format("unknown \"workspace.project.{}.type\" value", std::string(name)));

        if (proj.type == "interface") {
            proj.public_flags.insert(proj.public_flags.end(), flags.begin(), flags.end());
            proj.public_include_dirs.insert(proj.public_include_dirs.end(), include_dirs.begin(), include_dirs.end());
            proj.public_depends_on.insert(proj.public_depends_on.end(), depends_on.begin(), depends_on.end());
        } else {
            proj.private_flags.insert(proj.private_flags.end(), flags.begin(), flags.end());
            proj.private_include_dirs.insert(proj.private_include_dirs.end(), include_dirs.begin(), include_dirs.end());
            proj.private_depends_on.insert(proj.private_depends_on.end(), depends_on.begin(), depends_on.end());
        }

        // Normalize source and include dirs
        for (auto &expandable : sources)
            for (auto &src : expand_path((base_path / expandable).string())) {
                if (not fs::is_regular_file(src))
                    throw std::runtime_error(fmt::format("{} is not a regular file", src));

                proj.sources.push_back(fs::relative(src, base_path).string());
            }
        for (auto &dir : proj.private_include_dirs) {
            auto path = base_path / dir;
            if (not fs::is_directory(path))
                throw std::runtime_error(fmt::format("{} is not a directory", path.string()));

            dir = fs::absolute(path).string();
        }
        for (auto &dir : proj.public_include_dirs) {
            auto path = base_path / dir;
            if (not fs::is_directory(path))
                throw std::runtime_error(fmt::format("{} is not a directory", path.string()));

            dir = fs::absolute(path).string();
        }

        proj.base_path = base_path.string();
        w.project.emplace(std::string(name), std::move(proj));
    }

    // Resolve dependencies
    std::unordered_set<std::string> visited;
    for (auto &[name, _] : w.project) {
        std::vector<std::string> stack;
        resolve_dependencies(name, w.project, visited, stack);
    }

    generate_compile_commands(w, mode);

    return w;
}
