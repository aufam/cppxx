#include <fmt/ranges.h>
#include <filesystem>
#include <cstdlib>
#include "workspace.h"

namespace fs = std::filesystem;


auto Workspace::populate_archive(const std::string &uri_string) -> std::string {
    const fs::path uri = uri_string;
    const fs::path cache = cppxx_cache;
    const std::string extension = uri.extension().string();
    const bool is_remote = uri_string.starts_with("http://") or uri_string.starts_with("https://")
        or uri_string.starts_with("ftp://") or uri_string.starts_with("sftp://");
    const bool is_compressed = extension == ".zip" or extension == ".gz" or extension == ".bz2" or extension == ".xz";

    if (is_remote) {
        const fs::path archive_dir = cache / "archive" / encrypt(uri_string);
        const fs::path archive_path = archive_dir / uri.filename();

        if (not fs::exists(archive_path)) {
            fs::create_directories(archive_dir);
            const std::string cmd = fmt::format("curl -L -o '{}' '{}' > /dev/null 2>&1", archive_path.string(), uri_string);
            fmt::println(stderr, "[INFO] downloading {:?} to {:?}", uri_string, archive_path.string());
            if (int res = std::system(cmd.c_str()); res != 0)
                throw std::runtime_error(fmt::format("Failed to download archive from {:?}. Return code: {}", uri_string, res));
        }

        return populate_archive(archive_path.string());
    }

    if (is_compressed) {
        const fs::path extract_dir = cache / "extracted" / encrypt(uri_string);
        if (not fs::exists(extract_dir)) {
            fs::create_directories(extract_dir);
            std::string extract_cmd;
            if (extension == ".zip") {
                extract_cmd = fmt::format("unzip -q '{}' -d '{}'", uri_string, extract_dir.string());
            } else if (extension == ".gz") {
                extract_cmd = fmt::format("tar -xzf '{}' -C '{}'", uri_string, extract_dir.string());
            } else if (extension == ".bz2") {
                extract_cmd = fmt::format("tar -xjf '{}' -C '{}'", uri_string, extract_dir.string());
            } else if (extension == ".xz") {
                extract_cmd = fmt::format("tar -xJf '{}' -C '{}'", uri_string, extract_dir.string());
            } else {
                throw std::runtime_error(fmt::format("Unsupported archive type {:?}", extension));
            }

            fmt::println(stderr, "[INFO] extracting {:?} to {:?}", uri_string, extract_dir.string());
            if (int res = std::system(extract_cmd.c_str()); res != 0)
                throw std::runtime_error(fmt::format("Failed to extract {:?}. Return code: {}", uri_string, res));
        }

        return populate_archive(extract_dir.string());
    }

    if (not fs::exists(uri))
        throw std::runtime_error(fmt::format("{:?} does not exist or unresolvable", uri_string));

    return uri_string;
}
