#include <fmt/ranges.h>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


void Workspace::clear(const std::string &target) const {
    if (target.empty()) {
        fmt::println(stderr, "[WARNING] target {:?} is not specified", target);
        return;
    }

    auto it = targets.find(target);
    if (it == targets.end()) {
        fmt::println(stderr, "[WARNING] target {:?} does not exist", target);
        return;
    }

    auto &project = it->second;
    if (project.type == "interface") {
        fmt::println(stderr, "[WARNING] target {:?} is an interface", target);
        return;
    }

    for (auto &cc : project.compile_commands) {
        const fs::path out = cc.abs_output();
        if (fs::exists(out)) {
            fmt::println(stderr, "[INFO] clearing {:?}", out.string());
            fs::remove(out);
        }
    }
}
