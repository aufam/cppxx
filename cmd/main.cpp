#include <fmt/ranges.h>
#include <cxxopts.hpp>
#include <cppxx/cli/options.h>
#include "workspace.h"


struct Base {
    Base() = default;
    virtual ~Base() = default;

    virtual void exec() = 0;
};

// clang-format off
struct Run : Base {
    std::string file;

    Run(const std::string &name, int argc, char **argv) {
        cppxx::cli::parse( name, argc, argv, {
            {.target = &file, .key_char = 'f', .key_str = "file", .help = "Specify a cpp file", .is_positional = true},
        });
    }

    void exec() override { Workspace::exec(file, false); }
};

struct GenerateCompileCommands : Base {
    std::optional<std::string> file, root;

    GenerateCompileCommands(const std::string &name, int argc, char **argv) {
        cppxx::cli::parse(name, argc, argv, {
            { .target = &file, .key_char = 'f', .key_str = "file", .help = "A cpp file or this project if not specified", },
            { .target = &root, .key_str = "root", .help = "Specify root dir containing cppxx.toml", },
        });
    }

    void exec() override {
        if (file) {
            Workspace::exec(*file, true);
            return;
        }

        const auto w = Workspace::parse(root.value_or(""));
        w.generate_compile_commands_json();
    }
};

struct Build : Base {
    std::string target;
    std::string out;
    std::optional<int> threads = 2;
    std::optional<std::string> root;

    Build(const std::string &name, int argc, char **argv) {
        cppxx::cli::parse(name, argc, argv, {
            { .target = &target, .key_char = 't', .key_str = "target", .help = "Specify an executable or lib target", .is_positional = true, },
            { .target = &out, .key_char = 'o', .key_str = "out", .help = "Specify output name", },
            { .target = &threads, .key_char = 'j', .key_str = "threads", .help = "Number of threads", },
            { .target = &root, .key_str = "root", .help = "Specify root dir containing cppxx.toml", },
        });
    }

    void exec() override {
        const auto w = Workspace::parse(root.value_or(""));
        w.configure(*threads);
        w.build(target, out);
    }
};

struct Info : Base {
    std::optional<std::string> file, root;

    Info(const std::string &name, int argc, char **argv) {
        cppxx::cli::parse(name, argc, argv, {
            { .target = &root, .key_str = "root", .help = "Specify root dir containing cppxx.toml", },
        });
    }

    void exec() override {
        const auto w = Workspace::parse(root.value_or(""));
        w.print_info();
    }
};

struct Clear : Base {
    std::vector<std::string> target;
    std::optional<std::string> root;

    Clear(const std::string &name, int argc, char **argv) {
        cppxx::cli::parse(name, argc, argv, {
            { .target = &target, .key_char = 't', .key_str = "target", .help = "Specify target", .is_positional = true, },
            { .target = &root, .key_str = "root", .help = "Specify root dir containing cppxx.toml", },
        });
    }

    void exec() override {
        const auto w = Workspace::parse(root.value_or(""));
        for (auto &t : target)
            w.clear(t);
    }
};
// clang-format on

int main(int argc, char **argv) {
    std::string subcommand;
    const std::vector<cppxx::cli::Option> opts = {
        {.target = &subcommand,
         .key_str = "subcommand",
         .help = "Subcommand",
         .is_positional = true,
         .one_of = {"run", "build", "cc", "info", "clear"}}
    };
    cppxx::cli::parse("cppxx", std::min(argc, 2), argv, opts);

    std::unique_ptr<Base> app;
    if (subcommand == "run") {
        app = std::make_unique<Run>("Compile and run a cpp file in one go", argc - 1, argv + 1);
    } else if (subcommand == "build") {
        app = std::make_unique<Build>("Build a target specified by cppxx.toml", argc - 1, argv + 1);
    } else if (subcommand == "cc") {
        app = std::make_unique<GenerateCompileCommands>("Generate compile_commands.json", argc - 1, argv + 1);
    } else if (subcommand == "clear") {
        app = std::make_unique<Clear>("Clear build cache for specified targets", argc - 1, argv + 1);
    } else if (subcommand == "info") {
        app = std::make_unique<Info>("Print info as json string of current cppxx workspace", argc - 1, argv + 1);
    }

    try {
        app->exec();
    } catch (const std::exception &e) {
        fmt::println(stderr, "[ERROR] {}", e.what());
        return 1;
    }

    return 0;
}
