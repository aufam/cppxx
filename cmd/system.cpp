#include <fmt/ranges.h>
#include <cppxx/error.h>
#include "system.h"


std::expected<void, std::runtime_error> system(const std::string &cmd) {
    if (int status = std::system(cmd.c_str()); status == -1)
        return cppxx::unexpected_errorf("failed to run {:?}", cmd);

    else if (int res = WEXITSTATUS(status); WIFEXITED(status) and res != 0)
        return cppxx::unexpected_errorf("{:?} exited with return code {}", cmd, res);

    else if (int sig = WTERMSIG(status); WIFSIGNALED(status))
        return cppxx::unexpected_errorf("{:?} terminated by signal {}, exited with return code {}", cmd, sig, 128 + sig);

    return {};
}
