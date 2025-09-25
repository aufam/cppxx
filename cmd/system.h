#pragma once

#include <cstdlib>
#include <expected>
#include <stdexcept>


std::expected<void, std::runtime_error> system(const std::string &cmd);
