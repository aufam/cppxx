#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <regex>
#include <filesystem>
#include "git.h"
#include "system.h"

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

    // Handle https://[user[:password]@]github.com/user/repo
    static const std::regex https_pattern(R"(https?://(?:[^@]+@)?([^/]+)/(.+))");
    std::smatch https_match;
    if (std::regex_match(cleaned, https_match, https_pattern)) {
        return https_match[1].str() + "/" + https_match[2].str();
    }

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

std::string Git::as_key() const { return url + '@' + tag; }

std::expected<std::string, std::runtime_error> Git::clone(const std::string &cache) const {
    const std::string url = normalize_git_url(this->url);
    const std::string host = extract_host_and_path(url);

    fs::path result_path = fs::path(cache) / host / tag;
    if (fs::exists(result_path)) {
        // TODO: check if the result_path is dirty or needs update
        return result_path;
    }

    spdlog::info("cloning {}@{}", host, tag);
    const std::string cmd = fmt::format("git -c advice.detachedHead=false clone --quiet --depth 1 --branch '{}' '{}' '{}'", tag,
                                        url, result_path.string());

    if (auto res = system(cmd); not res)
        return cppxx::unexpected_errorf("Failed to clone repo from {:?}, {}", url, res.error().what());

    return result_path;
}
