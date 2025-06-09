#include <fmt/ranges.h>
#include "cli.h"
#include "workspace.h"


int main(int argc, char**argv) {
    std::string name;

    Option opts;
    opts.add_option(name, 'n', "name", "name", std::nullopt, true);
    opts.parse("cppxx", argc, argv);

    try {
        auto w = Workspace::parse();
        w.build(name);
    } catch (const std::exception &e) {
        fmt::println(stderr, "{}", e.what());
        return 1;
    }

    return 0;
}
