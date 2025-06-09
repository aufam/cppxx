#include <fmt/ranges.h>
#include "workspace.h"


void Workspace::assign_int(const toml::node &node, const std::string &key, int &value) {
    if (not node.is_integer())
        throw std::runtime_error(fmt::format("\"{}\" must be a string", key));

    value = node.value_or(0);
}

void Workspace::assign_string(const toml::node &node, const std::string &key, std::string &value) const {
    if (not node.is_string())
        throw std::runtime_error(fmt::format("\"{}\" must be a string", key));

    value = expand_variables(node.value_or(""));
}

void Workspace::assign_list(const toml::node &node,
                            const std::string &key,
                            std::vector<std::string> &private_values,
                            std::vector<std::string> &public_values,
                            std::vector<std::string> &unknown_values) {
    if (node.is_table()) {
        if (node.as_table()->contains("public"))
            for (auto &src : *node.as_table()->at("public").as_array())
                public_values.push_back(expand_variables(src.value_or("")));

        if (node.as_table()->contains("private"))
            for (auto &src : *node.as_table()->at("private").as_array())
                private_values.push_back(expand_variables(src.value_or("")));

        return;
    }

    if (not node.is_array())
        throw std::runtime_error(fmt::format("\"{}\" must be a list of strings", key));

    for (auto &src : *node.as_array())
        unknown_values.push_back(expand_variables(src.value_or("")));
}

void Workspace::assign_paths(const toml::node &node, const std::string &key, std::vector<std::string> &paths) {
    if (not node.is_array())
        throw std::runtime_error(fmt::format("\"{}\" must be a list of strings", key));

    for (auto &src : *node.as_array())
        paths.push_back(expand_variables(src.value_or("")));
}
