#include <fmt/ranges.h>
#include <json.hpp>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


int Workspace::exec(const std::string &file, bool generate_compile_commands) {
    fs::path cppxx_path;
    if (const char *cppxx_cache = std::getenv("CPPXX_CACHE"))
        cppxx_path = fs::weakly_canonical(cppxx_cache);
    else
        cppxx_path = ".cppxx";

    Workspace::CompileCommand cc;
    cc.file = fs::absolute(file).string();
    cc.directory = cppxx_path / "exec";
    cc.output = encrypt(cc.file) + "-" + fs::path(file).filename().stem().string();
    cc.command = fmt::format("c++ -std=c++23 -g -fsanitize=address,undefined "
                             "-Wall -Wextra -Wpedantic -Wnull-dereference -Wuse-after-free "
                             "-I{} -o {} {}",
                             cppxx_path.string(),
                             cc.output,
                             cc.file);

    if (generate_compile_commands) {
        nlohmann::json j = nlohmann::json::array();
        j.push_back({
            {"directory", cc.directory},
            {"file",      cc.file     },
            {"output",    cc.output   },
            {"command",   cc.command  },
        });

        std::ofstream out("compile_commands.json");
        out << j.dump(2);
        return 0;
    }

    std::error_code ec;
    auto output = fs::path(cc.directory) / fs::path(cc.output);
    auto file_time = fs::last_write_time(cc.file, ec);
    auto output_time = fs::last_write_time(output, ec);

    if (ec || !fs::exists(output) || file_time > output_time) {
        fs::create_directories(cc.directory);
        std::string cmd = fmt::format("cd {} && {}", cc.directory, cc.command);

        fmt::println(stderr, "[INFO] compiling {:?}", file);
        if (int status = WEXITSTATUS(std::system(cmd.c_str())))
            throw std::runtime_error(fmt::format("Command {:?} did not exit normally. Return code: {}", cc.command, status));

        fmt::println(stderr, "[INFO] running {:?}", file);
    }

    std::string cmd = fmt::format("{}", output.string());
    return WEXITSTATUS(std::system(cmd.c_str()));
}
