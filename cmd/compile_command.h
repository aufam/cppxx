#pragma once

#include <rfl.hpp>

struct CompileCommand {
    std::string file, directory, command, output;

    std::string get_abs_path() const;
};

struct CompileCommands {
    std::vector<CompileCommand> ccs;
    std::unordered_set<std::string> link_flags;
};
