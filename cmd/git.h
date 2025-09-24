#pragma once

#include <rfl.hpp>

struct Git {
    std::string url, tag;
    std::expected<std::string, std::runtime_error> clone(const std::string &cache) const;
    std::string as_key() const;
};
