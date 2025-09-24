#pragma once

#include <cxxopts.hpp>
#include <cppxx/cli/options.h>
#include <expected>


struct Base {
    Base() = default;
    virtual ~Base() = default;

    virtual std::expected<void, std::runtime_error> exec() = 0;
};

struct GenerateCompileCommands : Base {
    std::string target;
    std::optional<std::string> root;

    GenerateCompileCommands(const std::string &name, int argc, char **argv) {
        const std::vector<cppxx::cli::Option> options = {
            {
             .target = &target,
             .key_char = 't',
             .key_str = "target",
             .help = "Specify an executable or lib target",
             .is_positional = true,
             },
            {
             .target = &root,
             .key_str = "root",
             .help = "Specify root dir containing cppxx.toml",
             },
        };
        cppxx::cli::parse(name, argc, argv, options);
    }

    std::expected<void, std::runtime_error> exec() override;
};

struct Build : Base {
    std::string target;
    std::optional<int> jobs = 2;
    std::optional<std::string> out, root;

    Build(const std::string &name, int argc, char **argv) {
        const std::vector<cppxx::cli::Option> options = {
            {
             .target = &target,
             .key_char = 't',
             .key_str = "target",
             .help = "Specify an executable or lib target",
             .is_positional = true,
             },
            {
             .target = &out,
             .key_char = 'o',
             .key_str = "out",
             .help = "Specify output name",
             },
            {
             .target = &jobs,
             .key_char = 'j',
             .key_str = "threads",
             .help = "Number of threads",
             },
            {
             .target = &root,
             .key_str = "root",
             .help = "Specify root dir containing cppxx.toml",
             },
        };
        cppxx::cli::parse(name, argc, argv, options);
    }

    std::expected<void, std::runtime_error> exec() override;
};

struct Run : Base {
    std::string file;
    std::vector<std::string> args;

    Run(const std::string &name, int argc, char **argv) {
        const std::vector<cppxx::cli::Option> options = {
            {
             .target = &file,
             .key_char = 't',
             .key_str = "target",
             .help = "Specify an executable or lib target",
             .is_positional = true,
             },
        };
        cppxx::cli::parse(name, std::min(argc, 2), argv, options);

        argc -= 1;
        argv += 1;
        for (int i = 1; i < argc; ++i)
            args.emplace_back(argv[i]);
    }

    std::expected<void, std::runtime_error> exec() override;
};

struct Add : Base {
    std::optional<std::string> archive, git, tag;

    Add(const std::string &name, int argc, char **argv) {
        const std::vector<cppxx::cli::Option> options = {
            {
             .target = &archive,
             .key_str = "archive",
             .help = "Specify archive path or url",
             },
            {
             .target = &git,
             .key_str = "git",
             .help = "Specify git repository",
             },
            {
             .target = &tag,
             .key_str = "tag",
             .help = "Specify git tag",
             },
        };
        cppxx::cli::parse(name, argc, argv, options);
    }

    std::expected<void, std::runtime_error> exec() override;
};

struct Schema : Base {
    Schema(const std::string &name, int argc, char **argv) {
        const std::vector<cppxx::cli::Option> options = {};
        cppxx::cli::parse(name, argc, argv, options);
    }
    std::expected<void, std::runtime_error> exec() override;
};
