#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <cppxx/error.h>
#include "workspace.h"

namespace fs = std::filesystem;

// normalize Extended | vector<string> into a flat vector
static std::vector<std::string> normalize_variant(const std::optional<std::variant<Extended, std::vector<std::string>>> &opt) {
    if (!opt)
        return {};
    return std::visit(
        [](auto &&val) -> std::vector<std::string> {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, Extended>) {
                std::vector<std::string> result;
                result.insert(result.end(), val.public_().begin(), val.public_().end());
                result.insert(result.end(), val.private_().begin(), val.private_().end());
                return result;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                return val;
            }
            return {};
        },
        *opt);
}

// lookup a target by name in all maps
static const Target *find_target(const Workspace &ws, const std::string &name) {
    auto find_in = [&](const std::optional<std::unordered_map<std::string, Target>> &map) -> const Target * {
        if (not map)
            return nullptr;

        if (auto it = map->find(name); it != map->end())
            return &it->second;

        return nullptr;
    };
    if (auto t = find_in(ws.interface))
        return t;
    if (auto t = find_in(ws.lib))
        return t;
    if (auto t = find_in(ws.bin))
        return t;
    return nullptr;
}

// insert into new workspace, preserving category
static void insert_target(Workspace &dst, const std::string &name, const Target &t, const Workspace &src) {
    auto insert_in = [&](std::optional<std::unordered_map<std::string, Target>> &dstMap,
                         const std::optional<std::unordered_map<std::string, Target>> &srcMap) {
        if (!srcMap)
            return;
        if (srcMap->find(name) != srcMap->end()) {
            if (!dstMap)
                dstMap = std::unordered_map<std::string, Target>{};
            (*dstMap)[name] = t;
        }
    };
    insert_in(dst.interface, src.interface);
    insert_in(dst.lib, src.lib);
    insert_in(dst.bin, src.bin);
}


std::expected<Workspace, std::runtime_error> resolve_target(Workspace &&ws, const std::string &target_name) {
    Workspace result{
        .title = ws.title,
        .version = ws.version,
        .compiler = ws.compiler,
        .standard = ws.standard,
        .author = ws.author,
        .vars = std::nullopt,
        .interface = std::nullopt,
        .lib = std::nullopt,
        .bin = std::nullopt,
    };

    std::unordered_set<std::string> visited;         // fully processed targets
    std::unordered_set<std::string> recursion_stack; // targets in the current DFS path

    std::function<std::expected<void, std::runtime_error>(const std::string &)> dfs;
    dfs = [&](const std::string &name) -> std::expected<void, std::runtime_error> {
        if (recursion_stack.contains(name))
            return cppxx::unexpected_errorf("Failed to resolve target {:?}: circular dependency detected, stack = {}", name, recursion_stack);

        if (visited.contains(name))
            return {}; // already processed

        const Target *t = find_target(ws, name);
        if (not t)
            return cppxx::unexpected_errorf("Failed to resolve target {:?}: Target {:?} not found", target_name, name);

        recursion_stack.insert(name); // mark as in-progress
        insert_target(result, name, *t, ws);

        for (const auto &dep : normalize_variant(t->depends_on))
            if (auto r = dfs(dep); not r)
                return cppxx::unexpected_move(r);

        recursion_stack.erase(name); // done processing this target
        visited.insert(name);        // mark as fully processed

        return {};
    };

    return dfs(target_name).transform([&]() { return std::move(result); });
}
