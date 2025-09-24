#include <fmt/ranges.h>
#include <cppxx/error.h>
#include <cppxx/iterator.h>
#include <cppxx/match.h>
#include <rfl/json.hpp>
#include <sha256.h>
#include <filesystem>
#include "workspace.h"
#include "options.h"

namespace fs = std::filesystem;


static std::vector<std::string>
flatten_variant(const std::optional<std::variant<Extended, std::vector<std::string>>> &opt, bool public_only = false) {
    if (not opt)
        return {};
    if (std::holds_alternative<std::vector<std::string>>(*opt))
        return std::get<std::vector<std::string>>(*opt);
    if (std::holds_alternative<Extended>(*opt)) {
        const auto &ext = std::get<Extended>(*opt);

        std::vector<std::string> result;
        for (auto [t, dep] :
             std::array{
                 cppxx::zip(cppxx::repeat(true), ext.public_()),
                 cppxx::zip(cppxx::repeat(not public_only), ext.private_()),
             } | cppxx::join())
            if (t)
                result.push_back(dep);

        return result;
    }
    return {};
}

using RefTarget = std::reference_wrapper<const Target>;
using Collect = std::function<std::expected<void, std::runtime_error>(RefTarget)>;

static std::expected<RefTarget, std::runtime_error>
find_target(const Workspace &w, const std::string &name, bool is_dep = false) {
    if (w.interface && w.interface->contains(name))
        return w.interface->at(name);
    if (w.lib && w.lib->contains(name))
        return w.lib->at(name);
    if (w.bin && w.bin->contains(name)) {
        if (is_dep)
            return cppxx::unexpected_errorf("Bin target {:?} cannot be used as a dependency", name);
        else
            return w.bin->at(name);
    }
    return cppxx::unexpected_errorf("Target {:?} is not found", name);
};

static std::expected<CompileCommands, std::runtime_error>
generate_compile_commands_for_target(const Workspace &w, RefTarget target) {
    // TODO: c?
    const std::string compiler_standard = "-std=c++"
        + (std::holds_alternative<int>(w.standard) ? std::to_string(std::get<int>(w.standard))
                                                   : std::get<std::string>(w.standard));

    std::unordered_set<std::string> flags_set = {compiler_standard}, link_flags, visited;
    std::vector<std::string> flags = {compiler_standard};

    Collect collect = [&](RefTarget cur) -> std::expected<void, std::runtime_error> {
        for (auto [is_dir, f] :
             std::array{
                 cppxx::zip(cppxx::repeat(false), flatten_variant(cur.get().flags)),
                 cppxx::zip(cppxx::repeat(true), flatten_variant(cur.get().include_dirs)),
             } | cppxx::join())
            if (auto [it, ok] = flags_set.emplace(fmt::format("{}{}", is_dir ? "-I" : "", f)); ok)
                flags.push_back(*it);

        for (auto lf : cur.get().link_flags.value_or(std::vector<std::string>{}))
            link_flags.emplace(lf);

        for (auto &dep : flatten_variant(cur.get().depends_on, true)) {
            if (visited.contains(dep))
                continue;

            visited.emplace(dep);
            if (auto res = find_target(w, dep, true).and_then(collect); not res)
                return cppxx::unexpected_move(res);
        }

        return {};
    };

    return collect(target).transform([&]() -> CompileCommands {
        if (auto &srcs = *target.get().sources; target.get().sources) {
            return {
                .ccs = srcs | cppxx::map([&](const fs::path &file) {
                           const std::string command = fmt::format("{} {}", w.compiler, fmt::join(flags, " "));
                           CompileCommand cc = {};
                           cc.file = file;
                           cc.directory = fs::path(std::getenv(CPPXX_CACHE)) / "build";
                           cc.output = fmt::format("{}{}-{}.o", SHA256::hashString(command).substr(0, 8),
                                                   SHA256::hashString(file).substr(0, 8), file.filename().string());
                           cc.command = fmt::format("{} -o {} -c {}", command, cc.output, cc.file);

                           return cc;
                       })
                    | cppxx::collect<std::vector>(),
                .link_flags = std::move(link_flags),
            };
        } else {
            return {};
        }
    });
}

std::expected<CompileCommands, std::runtime_error> generate_compile_commands(const Workspace &w, const std::string &target_name) {
    std::unordered_map<std::string, CompileCommands> targets;

    auto add_target = [&](const std::string &name) {
        return [&](RefTarget t) {
            return generate_compile_commands_for_target(w, t).transform([&](CompileCommands &&cc) {
                targets.emplace(name, std::move(cc));
                return t;
            });
        };
    };

    Collect collect = [&](RefTarget cur) -> std::expected<void, std::runtime_error> {
        for (auto &dep : flatten_variant(cur.get().depends_on, true)) {
            if (targets.contains(dep))
                continue;

            if (auto res = find_target(w, dep, true).and_then(add_target(dep)).and_then(collect); not res)
                return cppxx::unexpected_move(res);
        }

        return {};
    };

    return find_target(w, target_name)
        .and_then(add_target(target_name))
        .and_then(collect)
        .transform([&]() {
            CompileCommands res;
            for (auto &[_, ccs] : targets) {
                std::ranges::move(ccs.ccs, std::back_inserter(res.ccs));
                res.link_flags.merge(ccs.link_flags);
            }
            return res;
        })
        .transform_error([&](std::runtime_error &&err) {
            return cppxx::errorf("Could not generate compile commands for target {:?}: {}", target_name, err.what());
        });
}

std::expected<void, std::runtime_error> GenerateCompileCommands::exec() {
    return Workspace::New(root.value_or(""))
        .and_then(resolve_vars)
        .and_then([&](Workspace &&w) { return resolve_target(std::move(w), target); })
        .and_then(resolve_remotes)
        .and_then(resolve_paths)
        .and_then([&](Workspace &&w) { return generate_compile_commands(w, target); })
        .transform([](const CompileCommands &ccs) {
            auto j = rfl::json::write(ccs.ccs, YYJSON_WRITE_PRETTY_TWO_SPACES);
            fmt::println("{}", j);
        });
}
