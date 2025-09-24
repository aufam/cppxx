#include <fmt/ranges.h>
#include <cppxx/error.h>
#include "workspace.h"
#include "options.h"

namespace fs = std::filesystem;

std::expected<void, std::runtime_error> Add::exec() {
    if (git and not tag)
        return cppxx::unexpected_errorf("{:?} must be specified", "--tag");

    return resolve_remotes(
               Workspace{
                   .compiler = "c++",
                   .standard = 23,
                   .interface =
                       std::unordered_map<std::string, Target>{
                           {"add", Target{.archive = archive, .git = git.transform([&](const std::string &url) -> Git {
                                              return Git{.url = url, .tag = *tag};
                                          })}}},
               })
        .transform([](auto &&) {});
}
