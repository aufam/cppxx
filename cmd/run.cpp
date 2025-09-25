#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <filesystem>
#include "workspace.h"
#include "options.h"
#include "system.h"

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

        if (auto res = system(fmt::format("{} {} {} -o {}", "c++ -std=c++23", fmt::join(flags, " "), file, output.string())); not res)
            return cppxx::unexpected_move(res);
    }

    args.insert(args.begin(), output.string());
    const std::string cmd = fmt::format("{}", fmt::join(args, " "));
    spdlog::info("running {:?}", cmd);

    return system(cmd);
}
