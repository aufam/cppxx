#include <fmt/ranges.h>
#include <json.hpp>
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

    for (auto &[name, target]: targets) {
        j[name]["type"] = target.type;
        j[name]["basePath"] = target.base_path;
        if (not target.sources.empty()) j[name]["sources"] = target.sources;
        if (not target.private_flags.empty()) j[name]["flags"]["private"] = target.private_flags;
        if (not target.public_flags.empty()) j[name]["flags"]["public"] = target.public_flags;
        if (not target.private_include_dirs.empty()) j[name]["includeDirs"]["private"] = target.private_include_dirs;
        if (not target.public_include_dirs.empty()) j[name]["includeDirs"]["public"] = target.public_include_dirs;
        if (not target.private_depends_on.empty()) j[name]["dependsOn"]["private"] = target.private_depends_on;
        if (not target.public_depends_on.empty()) j[name]["dependsOn"]["public"] = target.public_depends_on;
        if (not target.link_flags.empty()) j[name]["linkFlags"]["private"] = target.link_flags;
    }

    fmt::println("{}", j.dump(2));
}
