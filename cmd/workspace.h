#pragma once

#include "target.h"
#include "compile_command.h"

#define CPPXX_CACHE "CPPXX_CACHE"

struct Workspace {
    std::string title = "", version = "", compiler = "";
    std::variant<std::string, int> standard = "";
    std::string author = "";
    std::optional<std::unordered_map<std::string, std::string>> vars = std::nullopt;
    std::optional<std::unordered_map<std::string, Target>> interface = std::nullopt;
    std::optional<std::unordered_map<std::string, Target>> lib = std::nullopt;
    std::optional<std::unordered_map<std::string, Target>> bin = std::nullopt;

    static std::expected<Workspace, std::runtime_error> New(const std::string &root_dir = "");

    rfl::Skip<std::unordered_map<std::string, std::string>> populated = {};
};

std::expected<Workspace, std::runtime_error> resolve_vars(Workspace &&);
std::expected<Workspace, std::runtime_error> resolve_target(Workspace &&, const std::string &target);
std::expected<Workspace, std::runtime_error> resolve_remotes(Workspace &&);
std::expected<Workspace, std::runtime_error> resolve_paths(Workspace &&);

std::expected<CompileCommands, std::runtime_error> generate_compile_commands(const Workspace &, const std::string &target);
std::expected<void, std::runtime_error> build(CompileCommands &&, int jobs, const std::string &out);
std::expected<void, std::runtime_error> build_single(CompileCommands &&, const std::string &out);
