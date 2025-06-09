#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


void Workspace::populate_compile_commands(const std::string &mode) {
    const std::unordered_map<std::string, std::string> mode_map = {
        {"debug",   "-g -fsanitize=address,undefined"},
        {"release", "-O3 -DNDEBUG"                   }
    };

    if (not mode_map.contains(mode))
        throw std::runtime_error("Unknown build mode");

    fs::path cppxx_cache = this->cppxx_cache;
    for (auto &[_, project] : projects) {
        const fs::path base_path = project.base_path;
        for (auto &src : project.sources) {
            std::vector<std::string> flags;
            for (auto dir : project.private_include_dirs)
                flags.push_back("-I" + dir);
            for (auto dir : project.public_include_dirs)
                flags.push_back("-I" + dir);

            for (auto flag : project.private_flags)
                flags.push_back(flag);
            for (auto flag : project.public_flags)
                flags.push_back(flag);

            const std::string command = fmt::format("{} -std=c++{} {}", compiler, standard, fmt::join(flags, " "));
            const fs::path file = base_path / src;

            CompileCommand cc;
            cc.file = file.string();
            cc.output = src + ".o";

            cc.directory =
                (cc.file.starts_with(this->cppxx_cache) ? base_path / ".cppxx" / mode / encrypt(command)
                                                        : cppxx_cache / "build" / mode / title / version / encrypt(command))
                    .string();
            cc.command = fmt::format("{} {} -o {} -c {}", command, mode_map.at(mode), cc.output, cc.file);

            project.compile_commands.push_back(std::move(cc));
        }
    }
}

void Workspace::generate_compile_commands_json() const {
    nlohmann::json j;
    for (const auto &[_, project] : projects) {
        for (const auto &cc : project.compile_commands) {
            j.push_back({
                {"directory", cc.directory},
                {"file",      cc.file     },
                {"output",    cc.output   },
                {"command",   cc.command  },
            });
        }
    }

    std::ofstream out("compile_commands.json");
    out << j.dump(2);
}

auto Workspace::CompileCommand::abs_output() const -> std::string { return (fs::path(directory) / fs::path(output)).string(); }
