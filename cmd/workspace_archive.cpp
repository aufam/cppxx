#include <fmt/ranges.h>
#include <filesystem>
#include <cstdlib>
#include "workspace.h"

namespace fs = std::filesystem;

static std::string extract_archive_name(const std::string &url) {
    auto last_slash = url.find_last_of('/');
    return (last_slash == std::string::npos) ? url : url.substr(last_slash + 1);
}

static std::string sanitize_filename(const std::string &name) {
    std::string sanitized = name;
    std::replace(sanitized.begin(), sanitized.end(), '/', '_');
    return sanitized;
}

static std::string get_file_extension(const std::string &filename) {
    if (filename.ends_with(".tar.gz"))
        return "tar.gz";
    if (filename.ends_with(".tar.bz2"))
        return "tar.bz2";
    if (filename.ends_with(".tar.xz"))
        return "tar.xz";
    if (filename.ends_with(".zip"))
        return "zip";
    return "";
}

auto Workspace::populate_archive(const toml::node &node, const std::string &key) -> std::string {
    if (not node.is_string())
        throw std::runtime_error(fmt::format("{:?} must be a string", key));

    const std::string uri = expand_variables(node.value_or(""));
    const std::string type = get_file_extension(uri);
    const bool is_remote = uri.starts_with("http://") or uri.starts_with("https://");

    if (type.empty())
        throw std::runtime_error(fmt::format("Could not determine archive type for {:?}", uri));

    const std::string archive_name = extract_archive_name(uri);
    const std::string archive_dir = fmt::format("{}/archives/{}", cppxx_cache, sanitize_filename(archive_name));
    const std::string archive_path = is_remote ? fmt::format("{}/{}", archive_dir, archive_name) : fs::absolute(uri).string();

    if (not fs::exists(archive_path)) {
        fmt::println(stderr, "[INFO] downloading archive: {} > /dev/null 2>&1", uri);
        fs::create_directories(archive_dir);
        const std::string cmd = fmt::format("curl -L -o {} {}", archive_path, uri);
        if (int res = std::system(cmd.c_str()); res != 0)
            throw std::runtime_error(fmt::format("Failed to download archive from {:?}. Return code: {}", uri, res));
    }

    const std::string extract_dir = fmt::format("{}/extracted", archive_dir);
    if (not fs::exists(extract_dir)) {
        fs::create_directories(extract_dir);
        fmt::println(stderr, "[INFO] extracting archive: {}", archive_name);

        std::string extract_cmd;
        if (type == "zip") {
            extract_cmd = fmt::format("unzip -q {} -d {}", archive_path, extract_dir);
        } else if (type == "tar.gz") {
            extract_cmd = fmt::format("tar -xzf {} -C {} --strip-components=1", archive_path, extract_dir);
        } else if (type == "tar.bz2") {
            extract_cmd = fmt::format("tar -xjf {} -C {} --strip-components=1", archive_path, extract_dir);
        } else if (type == "tar.xz") {
            extract_cmd = fmt::format("tar -xJf {} -C {} --strip-components=1", archive_path, extract_dir);
        } else {
            throw std::runtime_error(fmt::format("Unsupported archive type '{}'", type));
        }

        if (int res = std::system(extract_cmd.c_str()); res != 0)
            throw std::runtime_error(fmt::format("Failed to extract archive {:?}. Return code: {}", archive_name, res));
    }

    return extract_dir;
}
