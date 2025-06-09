#include <fmt/ranges.h>
#include "workspace.h"


static void collect_deps(const Workspace &w, std::unordered_set<std::string> &deps, std::unordered_set<std::string> &flags, const std::string &name) {
    const auto &p = w.projects.at(name);

    for (auto &cc: p.compile_commands)
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

void Workspace::build(const std::string &name) const {
    auto it = projects.find(name);
    if (it == projects.end()) {
        fmt::println(stderr, "[ERROR] project {:?} does not exist", name);
        return;
    }

    auto &project = it->second;
    if (project.type == "interface") {
        fmt::println(stderr, "[WARNING] project {:?} is an interface", name);
        return;
    }

    std::unordered_set<std::string> deps, flags;
    collect_deps(*this, deps, flags, name);

    auto cmd = fmt::format("{} -std=c++{} {} {} -o {} > /dev/null 2>&1", compiler, standard, fmt::join(deps, " "), fmt::join(flags, " "), name);
    fmt::println(stderr, "[INFO] building {}", name);
    fmt::println(stderr, "{}", cmd);
    if (std::system(cmd.c_str()) != 0)
        throw std::runtime_error(fmt::format("Failed to build {}", name));
}
