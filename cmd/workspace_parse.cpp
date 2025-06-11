#include <fmt/ranges.h>
#include <filesystem>
#include "workspace.h"

namespace fs = std::filesystem;


auto Workspace::parse(std::string root_dir) -> Workspace {
    auto w = Workspace{};

    w.cppxx_cache = std::getenv("CPPXX_CACHE");
    if (w.cppxx_cache.empty())
        throw std::runtime_error("CPPXX_CACHE is not specified");

    w.cppxx_cache = fs::weakly_canonical(w.cppxx_cache).string();
    w.root_dir = fs::weakly_canonical(root_dir).string();

    auto config = toml::parse_file(root_dir + "cppxx.toml");

    if (not config.contains("workspace"))
        throw std::runtime_error("\"workspace\" table is missing");

    if (not config.at("workspace").is_table())
        throw std::runtime_error("\"workspace\" must be a table");

    for (auto &[key, node] : *config.at("workspace").as_table()) {
        if (key == "title") {
            w.assign_string(node, "workspace.title", w.title);
        } else if (key == "version") {
            w.assign_string(node, "workspace.version", w.version);
        } else if (key == "author") {
            w.assign_string(node, "workspace.author", w.author);
        } else if (key == "compiler") {
            w.assign_string(node, "workspace.compiler", w.compiler);
            if (w.linker.empty())
                w.linker = w.compiler;
        } else if (key == "linker") {
            w.assign_string(node, "workspace.linker", w.linker);
        } else if (key == "standard") {
            assign_int(node, "workspace.standard", w.standard);
        } else if (key == "vars") {
            w.assign_map(node, "workspace.vars", w.vars);
        } else {
            fmt::println(stderr, "[WARNING] unused key \"workspace.{}\"", std::string(key));
        }
    }

    if (w.title.empty())
        throw std::runtime_error("\"workspace.title\" must be specified");

    if (w.version.empty())
        throw std::runtime_error("\"workspace.version\" must be specified");

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

    if (not config.contains("project")) {
        fmt::println(stderr, "[WARNING] no project found");
        return w;
    }

    if (not config.at("project").is_table())
        throw std::runtime_error("\"project\" must be a table");

    for (auto &[name, project] : *config.at("project").as_table()) {
        if (not project.is_table())
            throw std::runtime_error(fmt::format("{:?} must be a table", "workspace.project." + std::string(name)));

        auto proj = Project{};
        std::vector<std::string> flags, link_flags, include_dirs, depends_on, sources;
        fs::path base_path = std::getenv("PWD");
        if (not root_dir.empty())
            base_path = base_path / root_dir;

        for (auto &[key, node] : *project.as_table()) {
            if (key == "type") {
                w.assign_string(node, fmt::format("workspace.project.{}.type", std::string(name)), proj.type);
            } else if (key == "sources") {
                w.assign_list(node, fmt::format("workspace.project.{}.sources", std::string(name)), sources);
            } else if (key == "flags") {
                w.assign_list(node,
                              fmt::format("workspace.project.{}.flags", std::string(name)),
                              flags,
                              &proj.public_flags,
                              &proj.private_flags);
            } else if (key == "include_dirs") {
                w.assign_list(node,
                              fmt::format("workspace.project.{}.include_dirs", std::string(name)),
                              include_dirs,
                              &proj.public_include_dirs,
                              &proj.private_include_dirs);
            } else if (key == "depends_on") {
                w.assign_list(node, fmt::format("workspace.project.{}.depends_on", std::string(name)), depends_on);
            } else if (key == "link_flags") {
                w.assign_list(node, fmt::format("workspace.project.{}.link_flags", std::string(name)), link_flags);
            } else if (key == "git") {
                base_path = w.populate_git(node, fmt::format("workspace.project.{}.git", std::string(name)));
            } else if (key == "archive") {
                std::string uri;
                w.assign_string(node, fmt::format("workspace.project.{}.archive", std::string(name)), uri);
                base_path = w.populate_archive(uri);
            } else {
                fmt::println(
                    stderr, "[WARNING] unused key {:?}", "workspace.project." + std::string(name) + "." + std::string(key));
            }
        }

        // Normalize type
        if (proj.type != "interface" and proj.type != "executable" and proj.type != "static" and proj.type != "dynamic")
            throw std::runtime_error(fmt::format("unknown {:?} value", "workspace.project." + std::string(name) + ".type"));

        if (proj.type == "interface") {
            proj.public_flags.insert(proj.public_flags.end(), flags.begin(), flags.end());
            proj.public_link_flags.insert(proj.public_link_flags.end(), link_flags.begin(), link_flags.end());
            proj.public_include_dirs.insert(proj.public_include_dirs.end(), include_dirs.begin(), include_dirs.end());
            proj.public_depends_on.insert(proj.public_depends_on.end(), depends_on.begin(), depends_on.end());
        } else {
            proj.private_flags.insert(proj.private_flags.end(), flags.begin(), flags.end());
            proj.private_link_flags.insert(proj.private_link_flags.end(), link_flags.begin(), link_flags.end());
            proj.private_include_dirs.insert(proj.private_include_dirs.end(), include_dirs.begin(), include_dirs.end());
            proj.private_depends_on.insert(proj.private_depends_on.end(), depends_on.begin(), depends_on.end());
        }

        proj.base_path = base_path.string();

        // Normalize source and include dirs
        // TODO: use populate archive
        for (auto &expandable : sources) {
            for (auto &src : expand_path((base_path / expandable).string())) {
                if (not fs::is_regular_file(src))
                    throw std::runtime_error(fmt::format("{:?} is not a regular file", src));

                proj.sources.push_back(fs::relative(src, base_path).string());
            }
        }
        for (auto &dir : proj.private_include_dirs) {
            auto path = base_path / dir;
            if (not fs::is_directory(path))
                throw std::runtime_error(fmt::format("{:?} is not a directory", path.string()));

            dir = fs::absolute(path).string();
        }
        for (auto &dir : proj.public_include_dirs) {
            auto path = base_path / dir;
            if (not fs::is_directory(path))
                throw std::runtime_error(fmt::format("{:?} is not a directory", path.string()));

            dir = fs::absolute(path).string();
        }

        w.projects.emplace(std::string(name), std::move(proj));
    }

    // Resolve dependencies
    std::unordered_set<std::string> visited;
    for (auto &[name, _] : w.projects) {
        std::vector<std::string> stack;
        w.resolve_dependencies(name, visited, stack);
    }

    w.populate_compile_commands();

    return w;
}
