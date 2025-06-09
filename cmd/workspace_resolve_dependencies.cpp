#include <fmt/ranges.h>
#include "workspace.h"


void Workspace::resolve_dependencies(const std::string &name,
                                     std::unordered_set<std::string> &visited,
                                     std::vector<std::string> &stack) {
    if (std::find(stack.begin(), stack.end(), name) != stack.end())
        throw std::runtime_error(fmt::format("Circular dependency detected: {}", name));

    if (visited.count(name))
        return;

    stack.push_back(name);
    auto &project = projects.at(name);

    for (const auto &dep : project.private_depends_on) {
        auto it = projects.find(dep);
        if (it == projects.end())
            throw std::runtime_error(fmt::format("\"workspace.project.{}\" is not found", dep));

        resolve_dependencies(dep, visited, stack);
        project.private_flags.insert(project.private_flags.end(), it->second.public_flags.begin(), it->second.public_flags.end());
        project.private_include_dirs.insert(
            project.private_include_dirs.end(), it->second.public_include_dirs.begin(), it->second.public_include_dirs.end());
    }

    for (const auto &dep : project.public_depends_on) {
        auto it = projects.find(dep);
        if (it == projects.end())
            throw std::runtime_error(fmt::format("\"workspace.project.{}\" is not found", dep));

        resolve_dependencies(dep, visited, stack);
        project.public_flags.insert(project.public_flags.end(), it->second.public_flags.begin(), it->second.public_flags.end());
        project.public_include_dirs.insert(
            project.public_include_dirs.end(), it->second.public_include_dirs.begin(), it->second.public_include_dirs.end());
    }

    stack.pop_back();
    visited.insert(name);
}
