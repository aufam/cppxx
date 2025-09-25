#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/defer.h>
#include <cppxx/error.h>
#include <filesystem>
#include <cstdlib>
#include <unordered_set>
#include "workspace.h"
#include "system.h"

namespace fs = std::filesystem;


static fs::path get_top_level_path_from_tar(const std::string &tar_file) {
    std::vector<std::string> result;
    std::unordered_set<std::string> unique_entries;

    std::string command = fmt::format("tar tf '{}' | cut -d/ -f1 | uniq", tar_file);
    auto pipe = popen(command.c_str(), "r");
    cppxx::defer _ = [&]() { pclose(pipe); };

    if (!pipe)
        throw std::runtime_error(fmt::format("Failed to run tar command for {:?}", tar_file));

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        if (line.empty())
            continue;
        if (line.back() == '\n')
            line.pop_back();
        if (unique_entries.insert(line).second)
            result.emplace_back(line);
    }

    if (result.empty())
        throw std::runtime_error(fmt::format("Failed to get the top level path from {:?}", tar_file));

    // TODO: what if the tar_file has multiple paths
    if (result.size() > 1)
        throw std::runtime_error(fmt::format("Multiple top level paths from {:?} are not supported. The paths are: {}", tar_file,
                                             fmt::join(result, " ")));

    return result.front();
}

static auto populate_archive(const fs::path &cache, const std::string &uri_string) -> std::string {
    const fs::path archive_dir = cache / "archive";
    const fs::path extract_dir = cache / "extracted";

    const fs::path uri = uri_string;
    const std::string extension = uri.extension().string();
    const bool is_remote = uri_string.starts_with("http://") or uri_string.starts_with("https://")
        or uri_string.starts_with("ftp://") or uri_string.starts_with("sftp://");
    const bool is_compressed = extension == ".tar" or extension == ".tgz" or extension == ".gz" or extension == ".tbz2"
        or extension == ".bz2" or extension == ".xz"; // TODO: zip?

    if (is_remote) {
        const fs::path archive_path = archive_dir / uri.filename();

        if (not fs::exists(archive_path)) {
            fs::create_directories(archive_dir);
            const std::string cmd = fmt::format("curl -sSfL -o '{}' '{}'", archive_path.string(), uri_string);
            spdlog::info("downloading {:?} to {:?}", uri_string, archive_path.string());
            if (auto res = system(cmd); not res)
                throw cppxx::errorf("Failed to download archive from {:?}, {}", uri_string, res.error().what());
        }

        return populate_archive(cache, archive_path.string());
    }

    if (is_compressed) {
        const fs::path extract_path = extract_dir / get_top_level_path_from_tar(uri_string);
        if (not fs::exists(extract_path)) {
            fs::create_directories(extract_dir);
            std::string extract_cmd;
            if (extension == ".tar") {
                extract_cmd = fmt::format("tar -xf '{}' -C '{}'", uri_string, extract_dir.string());
            } else if (extension == ".gz" or extension == ".tgz") {
                extract_cmd = fmt::format("tar -xzf '{}' -C '{}'", uri_string, extract_dir.string());
            } else if (extension == ".bz2" or extension == ".tbz2") {
                extract_cmd = fmt::format("tar -xjf '{}' -C '{}'", uri_string, extract_dir.string());
            } else if (extension == ".xz") {
                extract_cmd = fmt::format("tar -xJf '{}' -C '{}'", uri_string, extract_dir.string());
            } else {
                throw std::runtime_error(fmt::format("Unsupported archive type {:?}", uri_string));
            }

            spdlog::info("extracting {:?} to {:?}", uri_string, extract_dir.string());
            if (auto res = system(extract_cmd); not res)
                throw cppxx::errorf("Failed to extract {:?}, {}", uri_string, res.error().what());
        }

        return populate_archive(cache, extract_path.string());
    }

    if (uri.is_absolute() and not fs::exists(uri))
        throw std::runtime_error(fmt::format("{:?} does not exist or unresolvable", uri_string));

    // TODO: check existance of relative path?
    return uri_string;
}

static void populate_remotes(std::unordered_map<std::string, std::string> &populated,
                             const fs::path &cache,
                             const std::string &name,
                             Target &t) {
    if (t.archive and t.git)
        throw std::runtime_error(fmt::format(
            "multiple source detected from target {:?}: remote source could only be specified by {:?} or {:?} exclusively", name,
            "archive", "git"));
    if (t.archive)
        populated.emplace(t.archive.value(), populate_archive(cache, t.archive.value()));
    if (t.git) {
        auto key = t.git->as_key();
        if (not populated.contains(key)) {
            auto path = t.git->clone(cache);
            if (path)
                populated.emplace(key, path.value());
            else
                throw std::move(path.error());
        }
    }
    if (t.sources)
        for (auto &v : t.sources.value())
            populated.emplace(v, populate_archive(cache, v));
    if (t.include_dirs) {
        if (std::holds_alternative<Extended>(t.include_dirs.value())) {
            auto &e = std::get<Extended>(t.include_dirs.value());
            for (auto &path : e.public_())
                populated.emplace(path, populate_archive(cache, path));
            for (auto &path : e.private_())
                populated.emplace(path, populate_archive(cache, path));
        } else if (std::holds_alternative<std::vector<std::string>>(t.include_dirs.value())) {
            for (auto &path : std::get<std::vector<std::string>>(t.include_dirs.value()))
                populated.emplace(path, populate_archive(cache, path));
        }
    }
}

std::expected<Workspace, std::runtime_error> resolve_remotes(Workspace &&w) {
    fs::path cache;
    if (const char *cache_str = std::getenv(CPPXX_CACHE); not cache_str) {
        return std::unexpected(std::runtime_error(fmt::format("env variable for `${{{}}}` is not set", CPPXX_CACHE)));
    } else if (cache = cache_str; cache.is_relative()) {
        return std::unexpected(
            std::runtime_error(fmt::format("expect absolute path for `${{{}}}`, got {:?}", CPPXX_CACHE, cache.string())));
    }

    auto &populated = w.populated();
    try {
        if (w.interface)
            for (auto &[name, t] : w.interface.value())
                populate_remotes(populated, cache, name, t);
        if (w.lib)
            for (auto &[name, t] : w.lib.value())
                populate_remotes(populated, cache, name, t);
        if (w.bin)
            for (auto &[name, t] : w.bin.value())
                populate_remotes(populated, cache, name, t);
    } catch (std::runtime_error &e) {
        return cppxx::unexpected_errorf("Failed to resolve remotes: {}", e.what());
    }

    return std::move(w);
}
