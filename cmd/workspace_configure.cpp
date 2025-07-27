#include <fmt/ranges.h>
#include <filesystem>
#include <cppxx/iterator.h>
#include <cppxx/multithreading/channel.h>
#include "workspace.h"

namespace fs = std::filesystem;

void Workspace::configure(int threads) const {
    auto compile_commands = targets
        | cppxx::map([](auto &_, const Target& target) -> std::vector<CompileCommand> {
            return target.compile_commands
                | cppxx::map_filter([](const CompileCommand& cc) -> std::optional<CompileCommand> {
                    const fs::path file = cc.file;
                    const fs::path output = cc.abs_output();

                    std::error_code ec; // to avoid exceptions
                    auto file_time = fs::last_write_time(file, ec);
                    auto output_time = fs::last_write_time(output, ec);

                    // Push only if file is newer than output OR output does not exist
                    if (ec || !fs::exists(output) || file_time > output_time)
                        return cc;
                    return std::nullopt;
                })
                | cppxx::collect<std::vector>();
        })
        | cppxx::collect<std::vector>()
        | cppxx::join()
        | cppxx::collect<std::vector>();

    cppxx::multithreading::Channel<std::string> chan(threads);
    for (auto [i, cc] : compile_commands | cppxx::enumerate(1)) chan << [&, i]() -> std::string {
        fmt::println(stderr, "[INFO] [{}/{}] compiling {:?}", i, compile_commands.size(), cc.file);

        try {
            fs::create_directories(fs::path(cc.abs_output()).parent_path());
        } catch (std::runtime_error& e) {
            return e.what();
        }

        int status = std::system(("cd " + cc.directory + " && " + cc.command).c_str());
        if (!WIFEXITED(status) or WEXITSTATUS(status) != 0) {
            chan.terminate();
            return fmt::format("Command {:?} did not exit normally. Return code: {}", cc.command, status);
        }
        return "";
    };

    std::vector<std::string> errs;
    while (not chan.empty()) chan >> [&](std::string err) {
        if (not err.empty())
            errs.push_back(std::move(err));
    };

    if (not errs.empty()) {
        throw std::runtime_error(errs[0]);
    }
}
