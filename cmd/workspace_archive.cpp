#include <fmt/ranges.h>
#include <filesystem>
#include <cstdlib>
#include "workspace.h"

namespace fs = std::filesystem;


static fs::path get_top_level_path_from_tar(const std::string &tar_file) {
    std::vector<std::string> result;
    std::unordered_set<std::string> unique_entries;

    std::string command = "tar tf \"" + tar_file + "\" | cut -d/ -f1 | uniq";

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
        throw std::runtime_error(fmt::format("Failed to run tar command for {:?}", tar_file));

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe.get())) {
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
        throw std::runtime_error(fmt::format(
            "Multiple top level paths from {:?} are not supported. The paths are: {}", tar_file, fmt::join(result, " ")));

    return result.front();
}

auto Workspace::populate_archive(const std::string &uri_string) -> std::string {
    const fs::path cache = cppxx_cache;
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
            fmt::println(stderr, "[INFO] downloading {:?} to {:?}", uri_string, archive_path.string());
            if (int res = std::system(cmd.c_str()); res != 0)
                throw std::runtime_error(fmt::format("Failed to download archive from {:?}. Return code: {}", uri_string, res));
        }

        return populate_archive(archive_path.string());
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

            fmt::println(stderr, "[INFO] extracting {:?} to {:?}", uri_string, extract_dir.string());
            if (int res = std::system(extract_cmd.c_str()); res != 0)
                throw std::runtime_error(fmt::format("Failed to extract {:?}. Return code: {}", uri_string, res));
        }

        return populate_archive(extract_path.string());
    }

    if (uri.is_absolute() and not fs::exists(uri))
        throw std::runtime_error(fmt::format("{:?} does not exist or unresolvable", uri_string));

    // TODO: check existance of relative path?
    return uri_string;
}
