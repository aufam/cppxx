#include <fmt/ranges.h>
#include <nlohmann/json.hpp>
#include "workspace.h"


void Workspace::print_info() const {
    nlohmann::ordered_json j;

    if (not root_dir.empty()) j["rootDir"] = root_dir;
    if (not cppxx_cache.empty()) j["cppxxCache"] = cppxx_cache;
    if (not title.empty()) j["title"] = title;
    if (not version.empty()) j["version"] = version;
    if (not author.empty()) j["author"] = author;
    if (not compiler.empty()) j["compiler"] = compiler;
    if (standard) j["standard"] = standard;
    if (not vars.empty()) j["vars"] = vars;

    for (auto &[name, project]: projects) {
        j[name]["type"] = project.type;
        j[name]["basePath"] = project.base_path;
        if (not project.sources.empty()) j[name]["sources"] = project.sources;
        if (not project.private_flags.empty()) j[name]["flags"]["private"] = project.private_flags;
        if (not project.public_flags.empty()) j[name]["flags"]["public"] = project.public_flags;
        if (not project.private_link_flags.empty()) j[name]["linkFlags"]["private"] = project.private_link_flags;
        if (not project.public_link_flags.empty()) j[name]["linkFlags"]["public"] = project.public_link_flags;
        if (not project.private_include_dirs.empty()) j[name]["includeDirs"]["private"] = project.private_include_dirs;
        if (not project.public_include_dirs.empty()) j[name]["includeDirs"]["public"] = project.public_include_dirs;
        if (not project.private_depends_on.empty()) j[name]["dependsOn"]["private"] = project.private_depends_on;
        if (not project.public_depends_on.empty()) j[name]["dependsOn"]["public"] = project.public_depends_on;
    }

    fmt::println("{}", j.dump(2));
}
