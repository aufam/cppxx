#include <toml.hpp>
#include <unordered_map>
#include <unordered_set>


struct Workspace {
    std::string root_dir, cppxx_cache, title, version, author, compiler = "c++";
    int standard = 23;
    std::unordered_map<std::string, std::string> vars;

    struct CompileCommand {
        std::string directory, command, file, output;
        std::string abs_output() const;
    };

    struct Project {
        std::string type = "interface";
        std::string base_path;
        std::vector<std::string> sources;
        std::vector<std::string> public_flags;
        std::vector<std::string> public_link_flags;
        std::vector<std::string> public_include_dirs;
        std::vector<std::string> public_depends_on;
        std::vector<std::string> private_flags;
        std::vector<std::string> private_link_flags;
        std::vector<std::string> private_include_dirs;
        std::vector<std::string> private_depends_on;
        std::vector<CompileCommand> compile_commands;
    };

    std::unordered_map<std::string, Project> projects;

    static auto parse(std::string root_dir = "") -> Workspace;
    void configure() const;
    void build(const std::string &target, const std::string &out) const;
    void clear(const std::string &target) const;
    void generate_compile_commands_json() const;
    void print_info() const;

protected:
    auto expand_variables(const std::string &input) const -> std::string;
    static auto expand_path(const std::string &pattern) -> std::vector<std::string>;

    static void assign_int(const toml::node &node, const std::string &key, int &value);
    void assign_string(const toml::node &node, const std::string &key, std::string &value) const;
    void assign_list(const toml::node &node,
                     const std::string &key,
                     std::vector<std::string> &private_values,
                     std::vector<std::string> &public_values,
                     std::vector<std::string> &unknown_values);
    void assign_paths(const toml::node &node, const std::string &key, std::vector<std::string> &paths);

    auto populate_git(const toml::node &node, const std::string &key) -> std::string;

    void resolve_dependencies(const std::string &name,
                              std::unordered_set<std::string> &visited,
                              std::vector<std::string> &stack);

    static auto encrypt(const std::string &input, size_t length = 10) -> std::string;

    void populate_compile_commands();
};
