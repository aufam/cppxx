#include <fmt/ranges.h>
#include "workspace.h"


static void collect_deps(const Workspace &w,
                         std::unordered_set<std::string> &deps,
                         std::unordered_set<std::string> &flags,
                         const std::string &target) {
    const auto &p = w.targets.at(target);

    for (auto &cc : p.compile_commands)
        deps.emplace(cc.abs_output());

    for (auto &n : p.private_depends_on)
        collect_deps(w, deps, flags, n);

    for (auto &n : p.public_depends_on)
        collect_deps(w, deps, flags, n);

    for (auto &f : p.link_flags)
        flags.emplace(f);
}

void Workspace::build(const std::string &target, const std::string &out) const {
    auto it = targets.find(target);
    if (it == targets.end()) {
        fmt::println(stderr, "[WARNING] project {:?} does not exist", target);
        return;
    }

    auto &project = it->second;
    if (project.type == "interface") {
        fmt::println(stderr, "[WARNING] project {:?} is an interface", target);
        return;
    }

    std::unordered_set<std::string> deps, flags;
    collect_deps(*this, deps, flags, target);

    if (deps.empty())
        throw std::runtime_error(fmt::format("Object files for target {:?} are empty", target));

    std::string cmd;
    if (project.type == "executable") {
        cmd = fmt::format("{} {} {} -o {}", compiler, fmt::join(deps, " "), fmt::join(flags, " "), out);
    } else if (project.type == "dynamic") {
        cmd = fmt::format("{} -shared {} {} -o {}", compiler, fmt::join(deps, " "), fmt::join(flags, " "), out);
    } else if (project.type == "static") {
        // TODO: other archivers?
        cmd = fmt::format("ar rcs lib{}.a {}", out, fmt::join(deps, " "));
    } else {
        throw std::runtime_error(fmt::format("Unknown target type {:?}", project.type));
    }

    fmt::println(stderr, "[INFO] building {}", target);
    if (int res = std::system(cmd.c_str()); res != 0)
        throw std::runtime_error(fmt::format("Failed to build {:?} with command {:?} return code {}", target, cmd, res));
}
