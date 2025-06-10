#include <fmt/ranges.h>
#include <regex>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


static std::string extract_host_and_path(const std::string &url) {
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

static bool is_full_url(const std::string &url) {
    return url.starts_with("http://") || url.starts_with("https://") || url.starts_with("git@");
}

static bool has_host_prefix(const std::string &url) {
    // Check for common domain prefixes (gitlab.com/, bitbucket.org/, etc.)
    return url.find('.') != std::string::npos && url.find('/') != std::string::npos;
}

static std::string normalize_git_url(const std::string &url) {
    if (is_full_url(url))
        return url;

    if (has_host_prefix(url))
        return "https://" + url; // e.g., gitlab.com/user/repo

    // Otherwise assume it's a shorthand like user/repo
    return "https://github.com/" + url;
}

auto Workspace::populate_git(const toml::node &node, const std::string &key) -> std::string {
    if (not node.is_table() or not node.as_table()->contains("url") or not node.as_table()->contains("tag"))
        throw std::runtime_error(fmt::format("\"{}\" must be a table containing \"url\" and \"tag\"", key));

    const std::string url = normalize_git_url(expand_variables(node.as_table()->at("url").value_or("")));
    const std::string tag = expand_variables(node.as_table()->at("tag").value_or(""));
    const std::string host_and_path = extract_host_and_path(url);

    const std::string target_path = fmt::format("{}/{}/{}", cppxx_cache, host_and_path, tag);
    if (fs::exists(target_path))
        return target_path;

    fmt::println(stderr, "[INFO] cloning {}@{}", host_and_path, tag);
    const std::string cmd = fmt::format("git clone --depth 1 --branch {} {} {} > /dev/null 2>&1", tag, url, target_path);
    if (std::system(cmd.c_str()) != 0)
        throw std::runtime_error(fmt::format("Failed to clone repo from {}", url));

    return target_path;
}
