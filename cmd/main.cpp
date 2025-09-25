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

    enum struct Subcommand { build, run, compile_commands, add, schema };

    struct Opts {
        cppxx::cli::Tag<"subcommand,positional", Subcommand> subcommand;
    } opts;

    cppxx::cli::parse_tagged("cppxx", std::min(2, argc), argv, opts);

    auto argv0 = fmt::format("{} {}", argv[0], argv[1]);
    argv += 1;
    argv[0] = argv0.data();
    argc -= 1;

    return cppxx::match<std::expected<void, std::runtime_error>>(
               opts.subcommand(),
               {
                   {Subcommand::build,            [&]() { return Build("Build a target specified by cppxx.toml", argc, argv).exec(); }},
                   {Subcommand::run,              [&]() { return Run("Run a single cpp file", argc, argv).exec(); }                   },
                   {Subcommand::compile_commands,
                    [&]() { return GenerateCompileCommands("Generate compile_commands.json", argc, argv).exec(); }                    },
                   {Subcommand::add,              [&]() { return Add("Add an archive or a git repository", argc, argv).exec(); }      },
                   {Subcommand::schema,           [&]() { return Schema("Generate json schema for cppxx.json", argc, argv).exec(); }  },
    })
        .transform_error([](std::runtime_error &&e) {
            const std::string what = e.what();
            spdlog::error(what);

            std::regex re(R"(return code (\d+))");
            std::smatch match;
            return std::regex_search(what, match, re) ? std::stoi(match[1].str()) : -1;
        })
        .error_or(0);
}
