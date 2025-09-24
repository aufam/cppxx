#pragma once

#include <rfl.hpp>
#include "git.h"


struct Extended {
    rfl::Rename<"public", std::vector<std::string>> public_;
    rfl::Rename<"private", std::vector<std::string>> private_;
};

struct Target {
    std::optional<std::string> archive = std::nullopt;
    std::optional<Git> git = std::nullopt;
    std::optional<std::vector<std::string>> sources = std::nullopt;
    std::optional<std::variant<Extended, std::vector<std::string>>> include_dirs = std::nullopt;
    std::optional<std::variant<Extended, std::vector<std::string>>> flags = std::nullopt;
    std::optional<std::vector<std::string>> link_flags = std::nullopt;
    std::optional<std::variant<Extended, std::vector<std::string>>> depends_on = std::nullopt;
};
