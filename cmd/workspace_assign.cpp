#include <fmt/ranges.h>
#include "workspace.h"


void Workspace::assign_int(const toml::node &node, const std::string &key, int &value) {
    if (node.is_integer()) {
        value = node.value_or(0);
    } else if (node.is_string()) {
        const std::string val = node.value_or("");
        try {
            value = std::stoi(val);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(
                fmt::format("failed to assign int of field {:?}, cannot convert {:?} into an int", key, val));
        } catch (const std::out_of_range &e) {
            throw std::runtime_error(fmt::format("failed to assign int of field {:?}, value {:?} is out of range", key, val));
        }
    } else {
        throw std::runtime_error(fmt::format("{:?} must be an int or a string", key));
    }
}

void Workspace::assign_string(const toml::node &node, const std::string &key, std::string &value) const {
    if (not node.is_string())
        throw std::runtime_error(fmt::format("{:?} must be a string", key));

    value = expand_variables(node.value_or(""));
}

void Workspace::assign_list(const toml::node &node,
                            const std::string &key,
                            std::vector<std::string> &values,
                            std::vector<std::string> *public_values,
                            std::vector<std::string> *private_values) {
    if (public_values and private_values and node.is_table()) {
        bool has_public_or_private;
        if (node.as_table()->contains("public")) {
            has_public_or_private = true;
            for (auto &src : *node.as_table()->at("public").as_array())
                public_values->push_back(expand_variables(src.value_or("")));
        }

        if (node.as_table()->contains("private")) {
            has_public_or_private = true;

            for (auto &src : *node.as_table()->at("private").as_array())
                private_values->push_back(expand_variables(src.value_or("")));
        }

        if (not has_public_or_private)
            fmt::println(stderr, "[WARNING] field {:?} has no private nor public specifier", key);

        return;
    } else if (node.is_array()) {
        for (auto &src : *node.as_array())
            values.push_back(expand_variables(src.value_or("")));
    } else {
        throw std::runtime_error(fmt::format("{:?} must be a list of strings or a table with private and public fields", key));
    }
}

void Workspace::assign_map(const toml::node &node, const std::string &key, std::unordered_map<std::string, std::string> &map) {
    if (not node.is_table())
        throw std::runtime_error(fmt::format("{:?} must be a map of strings", key));

    for (auto &[k, v] : *node.as_table()) {
        const std::string sub_key(k);
        if (not v.is_string())
            throw std::runtime_error(fmt::format("{:?} must be a string", key + sub_key));

        map[sub_key] = expand_variables(v.value_or(""));
    }
}
