#include <fmt/ranges.h>
#include <cppxx/iterator.h>
#include "cli.h"
#include "workspace.h"


int main(int argc, char **argv) {
    std::vector<std::string> targets;
    std::string out;
    bool clear, cc, info;
    int threads;

    Option opts;
    opts.add_option(targets, 't', "targets",          "Specify target",                     "", true)
        .add_option(out,     'o', "out",              "Specify output file",                "")
        .add_option(clear,   'c', "clear",            "Specify targets to clear",           "false")
        .add_option(cc,      'g', "compile-commands", "Generate \"compile_commands.json\"", "false")
        .add_option(threads, 'j', "threads",          "Number of threads",                  "1")
        .add_option(info,    'i', "info",             "Print workspace info as json",       "false")
        .parse("cppxx", argc, argv);

    targets = targets
        | cppxx::filter([](std::string &target) { return not target.empty(); })
        | cppxx::collect<std::vector>();

    const std::string root_dir = "";
    try {
        const auto w = Workspace::parse(root_dir);
        if (cc) {
            w.generate_compile_commands_json();
        } else if (info) {
            w.print_info();
        } else if (clear) {
            for (auto &target : targets)
                w.clear(target);
        } else if (not out.empty()) {
            if (targets.size() == 0)
                throw std::runtime_error("no target specified");
            if (targets.size() > 1)
                throw std::runtime_error("multiple targets are not allowed when output is specified");

            w.configure(threads);
            w.build(targets.front(), out.empty() ? targets.front() : out);
        } else {
            w.configure(threads);
            for (auto &target : targets)
                w.build(target, target);
        }
    } catch (const std::exception &e) {
        fmt::println(stderr, "[ERROR] {}", e.what());
        return 1;
    }

    return 0;
}
