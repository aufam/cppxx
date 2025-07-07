#include <fmt/ranges.h>
#include <json.hpp>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


void Workspace::populate_compile_commands() {
    fs::path cache_path = cppxx_cache;
    for (auto &[_, target] : targets) {
        const fs::path base_path = target.base_path;
        for (auto &src : target.sources) {
            std::unordered_set<std::string> flags{
                fmt::format("-std=c++{}", standard),
            };

            for (auto dir : target.private_include_dirs)
                flags.emplace("-I" + dir);
            for (auto dir : target.public_include_dirs)
                flags.emplace("-I" + dir);

            for (auto flag : target.private_flags)
                flags.emplace(flag);
            for (auto flag : target.public_flags)
                flags.emplace(flag);

            const std::string command = fmt::format("{} {}", compiler, fmt::join(flags, " "));
            fs::path file = src;
            if (not file.is_absolute())
                file = base_path / file;

            CompileCommand cc;
            cc.file = file.string();
            cc.output = encrypt(command) + encrypt(cc.file) + "-" + file.filename().string() + ".o";
            cc.directory = cache_path / "build";
            cc.command = fmt::format("{} -o {} -c {}", command, cc.output, cc.file);

            target.compile_commands.push_back(std::move(cc));
        }
    }
}

void Workspace::generate_compile_commands_json() const {
    nlohmann::json j;
    for (const auto &[_, target] : projects) {
        for (const auto &cc : target.compile_commands) {
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

auto Workspace::CompileCommand::abs_output() const -> std::string {
    return (fs::path(directory) / fs::path(output)).string();
}
