#include <fmt/ranges.h>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;

void Workspace::configure() const {
    std::vector<CompileCommand> all;
    for (auto &[_, project] : projects) {
        for (auto &cc : project.compile_commands) {
            const fs::path file = cc.file;
            const fs::path output = cc.abs_output();

            std::error_code ec; // to avoid exceptions
            auto file_time = fs::last_write_time(file, ec);
            auto output_time = fs::last_write_time(output, ec);

            // Push only if file is newer than output OR output does not exist
            if (ec || !fs::exists(output) || file_time > output_time) {
                all.push_back(cc);
            }
        }
    }

    for (size_t i = 1; i <= all.size(); ++i) {
        const auto &cc = all[i - 1];
        fmt::println(stderr, "[{}/{}] {}", i, all.size(), cc.command);

        fs::create_directories(fs::path(cc.abs_output()).parent_path());
        int status = std::system(("cd " + cc.directory + " && " + cc.command).c_str());
        if (!WIFEXITED(status) or WEXITSTATUS(status) != 0) {
            throw std::runtime_error("Command did not exit normally\n");
        }
    }
}
