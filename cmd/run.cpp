#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <filesystem>
#include "workspace.h"
#include "options.h"

namespace fs = std::filesystem;


std::expected<void, std::runtime_error> Run::exec() {
    fs::path cache;
    if (auto env = std::getenv(CPPXX_CACHE); not env)
        return cppxx::unexpected_errorf("env varibale {:?} is not defined", CPPXX_CACHE);
    else
        cache = env;

    if (not fs::exists(file))
        return cppxx::unexpected_errorf("File {:?} does not exist", file);

    const Workspace w = {
        .compiler = "c++",
        .standard = 23,
        .bin =
            std::unordered_map<std::string, Target>{
                {file,
                 Target{
                     .sources = std::vector<std::string>{fs::absolute(fs::path(file)).string()},
                     .include_dirs = std::vector<std::string>{cache.string()},
                     .flags = std::vector<std::string>{"-g", "-fsanitize=address,undefined"},
                     .link_flags = std::vector<std::string>{"-fsanitize=address,undefined"},
                 }}},
    };

    const fs::path output = cache / "bin" / fs::path(file).filename().stem();

    return generate_compile_commands(w, file)
        .and_then([&](CompileCommands &&ccs) { return build(std::move(ccs), 1, output.string()); })
        .and_then([&]() -> std::expected<void, std::runtime_error> {
            const std::string cmd = fmt::format("{} {}", output.string(), fmt::join(args, " "));
            spdlog::info("running {:?}", cmd);

            if (int res = WEXITSTATUS(std::system(cmd.c_str())); res != 0)
                return cppxx::unexpected_errorf("{:?} exited with return code {}", file, res);
            else
                return {};
        });
}
