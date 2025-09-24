#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <rfl/toml.hpp>
#include <rfl/json.hpp>
#include <cppxx/error.h>
#include "workspace.h"

namespace fs = std::filesystem;


std::expected<Workspace, std::runtime_error> Workspace::New(const std::string &root_dir) {
    fs::path root = root_dir;
    fs::path file = root / "cppxx.toml";
    if (fs::exists(file)) {
        spdlog::debug("using {:?}", file.string());
    } else if (file = root / "cppxx.json"; fs::exists(file)) {
        spdlog::debug("using {:?}", file.string());
    } else {
        return cppxx::unexpected_errorf("Cannot find {:?} and {:?} in the root dir {:?}", "cppxx.toml", "cppxx.json", root_dir);
    }

    std::ifstream is(file);
    std::stringstream ss;
    ss << is.rdbuf();

    return (file.extension().string() == ".toml" ? rfl::toml::read<Workspace>(ss) : rfl::json::read<Workspace>(ss))
        .transform_error([&](rfl::Error &&err) { return cppxx::errorf("Cannot parse {:?}: {}", file.string(), err.what()); });
}
