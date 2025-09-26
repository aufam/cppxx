#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <filesystem>
#include <fstream>
#include "workspace.h"
#include "options.h"
#include "system.h"

namespace fs = std::filesystem;

inline std::string strip(const std::string &s, const std::string &chars = " \t\n\r\f\v") {
    auto start = s.find_first_not_of(chars);
    if (start == std::string::npos)
        return ""; // all chars are stripped

    auto end = s.find_last_not_of(chars);
    return s.substr(start, end - start + 1);
}

std::expected<void, std::runtime_error> Run::exec() {
    fs::path cache;
    if (auto env = std::getenv(CPPXX_CACHE); not env)
        return cppxx::unexpected_errorf("env varibale {:?} is not defined", CPPXX_CACHE);
    else
        cache = env;

    if (not fs::exists(file))
        return cppxx::unexpected_errorf("File {:?} does not exist", file);

    std::ifstream f(file);
    if (not f.is_open())
        return cppxx::unexpected_errorf("File {:?} is not readable", file);

    std::string additional_flags;
    std::string compiler;
    std::string standard;

    for (std::string line; std::getline(f, line);) {
        if (not line.starts_with("//"))
            break;

        std::string comment = strip(line.substr(2));
        if (constexpr std::string_view key = "flags:"; comment.starts_with(key) && additional_flags.empty()) {
            additional_flags = strip(comment.substr(key.size()));
        } else if (constexpr std::string_view key = "compiler:"; comment.starts_with(key) && compiler.empty()) {
            compiler = strip(comment.substr(key.size()));
        } else if (constexpr std::string_view key = "standard:"; comment.starts_with(key) && standard.empty()) {
            standard = strip(comment.substr(key.size()));
        }
    }
    f.close();

    if (compiler.empty())
        compiler = "c++";
    if (standard.empty())
        standard = "23";

    const fs::path output_dir = cache / "bin";
    const fs::path output = output_dir / fs::path(file).filename().stem();
    fs::create_directories(output_dir);

    if (not fs::exists(output) or fs::last_write_time(file) > fs::last_write_time(output)) {
        spdlog::info("compiling {:?}", file);
        std::vector<std::string> flags = {additional_flags, "-g", "-fsanitize=address,undefined", "-I" + cache.string()};

        if (auto res = system(
                fmt::format("{} -std=c++{} {} {} -o {}", compiler, standard, fmt::join(flags, " "), file, output.string()));
            not res)
            return cppxx::unexpected_move(res);
    }

    args.insert(args.begin(), output.string());
    const std::string cmd = fmt::format("{}", fmt::join(args, " "));
    spdlog::info("running {:?}", cmd);

    return system(cmd);
}
