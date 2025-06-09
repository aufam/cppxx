#include <fmt/ranges.h>
#include "workspace.h"


static void collect_deps(const Workspace &w,
                         std::unordered_set<std::string> &deps,
                         std::unordered_set<std::string> &flags,
                         const std::string &target) {
    const auto &p = w.projects.at(target);

    for (auto &cc : p.compile_commands)
        deps.emplace(cc.abs_output());

    for (auto &f : p.private_link_flags)
        flags.emplace(f);

    for (auto &f : p.public_link_flags)
        flags.emplace(f);

    for (auto &n : p.private_depends_on)
        collect_deps(w, deps, flags, n);

    for (auto &n : p.public_depends_on)
        collect_deps(w, deps, flags, n);
}

void Workspace::build(const std::string &target, const std::string &out) const {
    const std::unordered_map<std::string, std::string> mode_map = {
        {"debug",   "-fsanitize=address,undefined"},
        {"release", "-O3"                         }
    };

    auto it = projects.find(target);
    if (it == projects.end()) {
        fmt::println(stderr, "[ERROR] project {:?} does not exist", target);
        return;
    }

    auto &project = it->second;
    if (project.type == "interface") {
        fmt::println(stderr, "[WARNING] project {:?} is an interface", target);
        return;
    }

    std::unordered_set<std::string> deps, flags;
    collect_deps(*this, deps, flags, target);

    auto cmd = fmt::format("{} -std=c++{} {} {} {} -o {}",
                           compiler,
                           standard,
                           mode_map.at("debug"),
                           fmt::join(deps, " "),
                           fmt::join(flags, " "),
                           out);
    fmt::println(stderr, "[INFO] building {}", target);
    if (std::system(cmd.c_str()) != 0)
        throw std::runtime_error(fmt::format("Failed to build {}", target));
}
