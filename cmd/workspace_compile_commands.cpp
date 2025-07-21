#include <fmt/ranges.h>
#include <json.hpp>
#include <filesystem>
#include <ranges>
#include "workspace.h"

namespace fs = std::filesystem;


void Workspace::populate_compile_commands() {
    fs::path cache_path = cppxx_cache;
    for (auto &[_, target] : targets) {
        const fs::path base_path = target.base_path;
        for (auto &src : target.sources) {
            std::unordered_set<std::string> flags_set = {fmt::format("-std=c++{}", standard)};
            std::vector<std::string> flags = {fmt::format("-std=c++{}", standard)};

            for (auto [is_inc_dir, item] :
                 std::array{
                     std::views::zip(std::views::repeat(true), target.private_include_dirs),
                     std::views::zip(std::views::repeat(true), target.public_include_dirs),
                     std::views::zip(std::views::repeat(false), target.private_flags),
                     std::views::zip(std::views::repeat(false), target.public_flags),
                 } | std::views::join) {
                const auto flag = is_inc_dir ? "-I" + item : item;
                const auto [_, ok] = flags_set.emplace(flag);
                if (ok)
                    flags.push_back(flag);
            }

            const std::string command = fmt::format("{} {}", compiler, fmt::join(flags, " "));
            fs::path file = src;

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
    for (const auto &[_, target] : targets) {
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

auto Workspace::CompileCommand::abs_output() const -> std::string { return (fs::path(directory) / fs::path(output)).string(); }
