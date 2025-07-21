#include <fmt/ranges.h>
#include <ranges>
#include "workspace.h"


static void resolve(Workspace &w,
                    const std::string &target_name,
                    Workspace::Target &target,
                    std::unordered_set<std::string> &visited,
                    std::vector<std::string> &stack) {
    if (std::find(stack.begin(), stack.end(), target_name) != stack.end())
        throw std::runtime_error(
            fmt::format("Circular dependency detected in target {:?}. The dependencies are {}", target_name, stack));

    if (visited.count(target_name))
        return;

    stack.push_back(target_name);

    for (auto [is_public, dep_name] :
         std::array{
             std::views::zip(std::views::repeat(false), target.private_depends_on),
             std::views::zip(std::views::repeat(true), target.public_depends_on),
         } | std::views::join) {
        auto it = w.targets.find(dep_name);
        if (it == w.targets.end())
            throw std::runtime_error(fmt::format("{:?} which is required by {:?} is not found", dep_name, target_name));

        auto &dep = it->second;
        resolve(w, dep_name, dep, visited, stack);

        std::ranges::copy(dep.public_flags,
                          is_public ? std::back_inserter(target.public_flags) : std::back_inserter(target.private_flags));

        std::ranges::copy(dep.public_include_dirs,
                          is_public ? std::back_inserter(target.public_include_dirs)
                                    : std::back_inserter(target.private_include_dirs));
    }

    stack.pop_back();
    visited.insert(target_name);
}

void Workspace::resolve_dependencies() {
    std::unordered_set<std::string> visited;
    for (auto &[name, target] : targets) {
        std::vector<std::string> stack;
        resolve(*this, name, target, visited, stack);
    }
}
