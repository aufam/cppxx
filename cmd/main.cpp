#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <cppxx/iterator.h>
#include <cppxx/debug.h>
#include "common.h"

namespace fs = std::filesystem;

void execute_command(const CompileCommands &cc) {
    fs::path output_path = cc.directory + "/" + cc.output;
    fs::create_directories(output_path.parent_path());
    int status = system(("cd " + cc.directory + " && " + cc.command).c_str());
    if (!WIFEXITED(status) or WEXITSTATUS(status) != 0) {
        throw std::runtime_error("Command did not exit normally\n");
    }
}

int main() {
    try {
        Workspace w = parse_workspace();

        std::vector<CompileCommands> all;
        for (auto &[_, project] : w.project) {
            for (auto &cc : project.compile_commands) {
                fs::path file = cc.file;
                fs::path output = cc.directory + "/" + cc.output;

                std::error_code ec; // to avoid exceptions
                auto file_time = fs::last_write_time(file, ec);
                auto output_time = fs::last_write_time(output, ec);

                // Push only if file is newer than output OR output does not exist
                if (ec || !fs::exists(output) || file_time > output_time) {
                    all.push_back(cc);
                }
            }
        }

        for (auto [i, c] : all | cppxx::enumerate(1)) {
            fmt::println(stderr, "[{}/{}] {}", i, all.size(), c.command);
            execute_command(c);
        }
    } catch (const std::exception &e) {
        fmt::println(stderr, "{}", e.what());
        return 1;
    }

    return 0;
}
