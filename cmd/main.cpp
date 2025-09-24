#include <fmt/ranges.h>
#include <rfl/toml.hpp>
#include <rfl/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <cppxx/match.h>
#include "options.h"


int main(int argc, char **argv) {
    spdlog::set_default_logger(spdlog::stderr_color_mt("cppxx"));
    spdlog::set_pattern("[%^%l%$] %v");

    std::string subcommand;
    const std::vector<cppxx::cli::Option> opts = {
        {.target = &subcommand,
         .key_str = "subcommand",
         .help = "Subcommand",
         .is_positional = true,
         .one_of = {"build", "run", "cc", "add", "schema"}}
    };

    cppxx::cli::parse("cppxx", std::min(2, argc), argv, opts);

    auto argv0 = fmt::format("{} {}", argv[0], argv[1]);
    argv += 1;
    argv[0] = argv0.data();
    argc -= 1;

    const auto res = cppxx::match<std::expected<void, std::runtime_error>>(
        subcommand,
        {
            {"build",  [&]() { return Build("Build a target specified by cppxx.toml", argc, argv).exec(); }          },
            {"run",    [&]() { return Run("Run a single cpp file", argc, argv).exec(); }                             },
            {"cc",     [&]() { return GenerateCompileCommands("Generate compile_commands.json", argc, argv).exec(); }},
            {"add",    [&]() { return Add("Add an archive or a git repository", argc, argv).exec(); }                },
            {"schema", [&]() { return Schema("Generate json schema for cppxx.json", argc, argv).exec(); }            },
    });

    if (not res) {
        spdlog::error("{}", res.error().what());
        return 1;
    }

    return 0;
}
