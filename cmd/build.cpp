#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include <cppxx/iterator.h>
#include <cppxx/multithreading/channel.h>
#include <filesystem>
#include "workspace.h"
#include "options.h"

namespace fs = std::filesystem;


std::expected<void, std::runtime_error> build(CompileCommands &&ccs, int jobs, const std::string &out) {
    auto filtered = ccs.ccs | cppxx::filter([](const CompileCommand &cc) {
                        std::error_code ec; // to avoid exceptions
                        auto file_time = fs::last_write_time(cc.file, ec);
                        auto output_time = fs::last_write_time(cc.get_abs_path(), ec);

                        return ec || !fs::exists(cc.get_abs_path()) || file_time > output_time;
                    })
        | cppxx::collect<std::vector>();

    cppxx::multithreading::Channel<std::expected<void, std::runtime_error>> chan(jobs);
    for (auto [i, cc] : filtered | cppxx::enumerate(1))
        chan << [&, i]() -> std::expected<void, std::runtime_error> {
            spdlog::info("[{}/{}] compiling {:?}", i, filtered.size(), cc.file);

            try {
                fs::create_directories(fs::path(cc.get_abs_path()).parent_path());
            } catch (std::runtime_error &e) {
                return cppxx::unexpected_errorf("Failed to compile {:?}: {}", cc.file, e.what());
            }

            int status = std::system(("cd " + cc.directory + " && " + cc.command).c_str());
            if (!WIFEXITED(status) or WEXITSTATUS(status) != 0) {
                chan.terminate();
                return cppxx::unexpected_errorf("Command {:?} did not exit normally. Return code: {}", cc.command, status);
            }
            return {};
        };

    std::optional<std::runtime_error> err = std::nullopt;
    while (not chan.empty())
        chan >> [&](std::expected<void, std::runtime_error> res) {
            if (not res)
                err.emplace(std::move(res.error()));
        };

    if (err)
        return std::unexpected(std::move(*err));


    // TODO: static and shared libs?
    const auto deps = ccs.ccs | cppxx::map([](const CompileCommand &cc) { return cc.get_abs_path(); });
    const auto cmd = fmt::format("{} {} {} -o {}", "c++", fmt::join(deps, " "), fmt::join(ccs.link_flags, " "), out);

    spdlog::info("building {}", out);
    if (int res = std::system(cmd.c_str()); res != 0)
        return cppxx::unexpected_errorf("Failed to build {:?} with command {:?} return code {}", out, cmd, res);

    return {};
}


std::expected<void, std::runtime_error> Build::exec() {
    if (not out)
        out = target;

    return Workspace::New(root.value_or(""))
        .and_then(resolve_vars)
        .and_then([&](Workspace &&w) { return resolve_target(std::move(w), target); })
        .and_then(resolve_remotes)
        .and_then(resolve_paths)
        .and_then([&](Workspace &&w) { return generate_compile_commands(w, target); })
        .and_then([&](CompileCommands &&ccs) { return build(std::move(ccs), *jobs, *out); });
}
