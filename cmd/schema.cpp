#include <fmt/ranges.h>
#include <rfl/json.hpp>
#include "workspace.h"
#include "options.h"

std::expected<void, std::runtime_error> Schema::exec() {
    auto j = rfl::json::to_schema<Workspace>(YYJSON_WRITE_PRETTY_TWO_SPACES);
    fmt::println("{}", j);
    return {};
}
