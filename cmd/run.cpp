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

    const fs::path output_dir = cache / "bin";
    const fs::path output = output_dir / fs::path(file).filename().stem();
    fs::create_directories(output_dir);

    if (not fs::exists(output) or fs::last_write_time(file) > fs::last_write_time(output)) {
        spdlog::info("compiling {:?}", file);
        std::vector<std::string> flags = {"-g", "-fsanitize=address,undefined", "-I" + cache.string()};
        auto cmd = fmt::format("{} {} {} -o {}", "c++ -std=c++23", fmt::join(flags, " "), file, output.string());
        if (int res = WEXITSTATUS(std::system(cmd.c_str())); res != 0)
            return cppxx::unexpected_errorf("Failed to build {:?} with command {:?} return code {}", output.string(), cmd, res);
    }

    const std::string cmd = fmt::format("{} {}", output.string(), fmt::join(args, " "));
    spdlog::info("running {:?}", cmd);
    if (int res = WEXITSTATUS(std::system(cmd.c_str())); res != 0)
        return cppxx::unexpected_errorf("{:?} exited with return code {}", file, res);
    else
        return {};
}
