#include <fmt/ranges.h>
#include <filesystem>
#include <ranges>
#include "workspace.h"

namespace fs = std::filesystem;


auto Workspace::parse(std::string root_dir) -> Workspace {
    auto w = Workspace{};
    if (const char *cppxx_cache = std::getenv("CPPXX_CACHE"))
        w.cppxx_cache = fs::weakly_canonical(cppxx_cache);
    else
        w.cppxx_cache = ".cppxx";

    const fs::path root = fs::weakly_canonical(root_dir);
    w.root_dir = root;

    auto config = toml::parse_file((root / "cppxx.toml").string());
    const std::string workspace = "workspace";

    if (not config.contains(workspace))
        throw std::runtime_error(fmt::format("{:?} table is missing", workspace));

    if (not config.at(workspace).is_table())
        throw std::runtime_error(fmt::format("{:?} must be a table", workspace));

    for (auto &[key, node] : *config.at(workspace).as_table()) {
        const std::string node_key = workspace + "." + std::string(key);
        if (key == "title") {
            w.assign_string(node, node_key, w.title);
        } else if (key == "version") {
            w.assign_string(node, node_key, w.version);
        } else if (key == "author") {
            w.assign_string(node, node_key, w.author);
        } else if (key == "compiler") {
            w.assign_string(node, node_key, w.compiler);
            if (w.linker.empty())
                w.linker = w.compiler;
        } else if (key == "linker") {
            w.assign_string(node, node_key, w.linker);
        } else if (key == "standard") {
            w.assign_int(node, node_key, w.standard);
        } else if (key == "vars") {
            w.assign_map(node, node_key, w.vars);
        } else {
            fmt::println(stderr, "[WARNING] unused key {:?}", node_key);
        }
    }

    if (w.title.empty())
        throw std::runtime_error(fmt::format("{:?} must be specified", workspace + ".title"));

    if (w.version.empty())
        throw std::runtime_error(fmt::format("{:?} must be specified", workspace + ".version"));

    switch (w.standard) {
    case 11:
    case 14:
    case 17:
    case 20:
    case 23:
    case 26:
        break;
    default:
        throw std::runtime_error(fmt::format("unknown c++ standard \"{}\"", w.standard));
    }

    config.erase(workspace);
    if (config.empty()) {
        fmt::println(stderr, "[WARNING] no target found");
        return w;
    }

    for (const auto &[target_key, target_node] : config) {
        const auto target_name = std::string(target_key);
        if (not target_node.is_table())
            throw std::runtime_error(fmt::format("{:?} must be a table", target_name));

        auto target = Target{};
        std::vector<std::string> flags, include_dirs, depends_on, sources;
        fs::path base_path = std::getenv("PWD");
        if (not root_dir.empty())
            base_path = base_path / root_dir; // TODO

        for (auto &[key, node] : *target_node.as_table()) {
            const auto node_key = target_name + "." + std::string(key);
            if (key == "type") {
                w.assign_string(node, node_key, target.type);
            } else if (key == "sources") {
                w.assign_list(node, node_key, sources);
            } else if (key == "flags") {
                w.assign_list(node, node_key, flags, &target.public_flags, &target.private_flags);
            } else if (key == "include_dirs") {
                w.assign_list(node, node_key, include_dirs, &target.public_include_dirs, &target.private_include_dirs);
            } else if (key == "depends_on") {
                w.assign_list(node, node_key, depends_on);
            } else if (key == "link_flags") {
                w.assign_list(node, node_key, target.link_flags);
            } else if (key == "git") {
                base_path = w.populate_git(node, node_key);
            } else if (key == "archive") {
                std::string uri;
                w.assign_string(node, node_key, uri);
                base_path = w.populate_archive(uri);
            } else {
                fmt::println(stderr, "[WARNING] unused key {:?}", node_key);
            }
        }

        // Normalize type
        const std::vector<std::string> types = {"interface", "executable", "static", "dynamic"};
        if (std::find(types.begin(), types.end(), std::string(target.type)) == types.end())
            throw std::runtime_error(
                fmt::format("Unknown type for {:?}. Expect one of {}, got {:?}", target_name, types, target.type));

        if (target.is_interface()) {
            std::ranges::move(flags, std::back_inserter(target.public_flags));
            std::ranges::move(include_dirs, std::back_inserter(target.public_include_dirs));
            std::ranges::move(depends_on, std::back_inserter(target.public_depends_on));
        } else {
            std::ranges::move(flags, std::back_inserter(target.private_flags));
            std::ranges::move(include_dirs, std::back_inserter(target.private_include_dirs));
            std::ranges::move(depends_on, std::back_inserter(target.private_depends_on));
        }

        target.base_path = base_path.string();

        // Normalize source and include dirs
        for (const auto &src : sources) {
            std::vector<std::string> expanded;
            if (src.find('*') != std::string::npos) {
                fs::path path_to_expand = src;
                if (not path_to_expand.is_absolute())
                    path_to_expand = base_path / path_to_expand;

                expanded = expand_path(path_to_expand);
            } else {
                expanded.push_back(w.populate_archive(src));
            }

            for (fs::path path : expanded) {
                std::string extension = path.extension();
                if (extension == ".cpp" or extension == ".cxx" or extension == ".c" or extension == ".cc") {
                    target.sources.push_back(path);
                } else if (extension == ".h" or extension == ".hpp") {
                    if (target.is_interface())
                        target.public_include_dirs.push_back(path.parent_path());
                    else
                        target.private_include_dirs.push_back(path.parent_path());
                }
            }
        }

        for (auto &dir : std::array{std::views::all(target.private_include_dirs), std::views::all(target.public_include_dirs)}
                 | std::views::join) {
            fs::path path = dir;
            if (not path.is_absolute())
                path = fs::absolute(base_path) / path;

            if (not fs::is_directory(path))
                throw std::runtime_error(fmt::format("{:?} is not a directory", path.string()));

            dir = path;
        }

        w.targets.emplace(target_name, std::move(target));
    }

    w.resolve_dependencies();
    w.populate_compile_commands();

    return w;
}
