#include <toml.hpp>
#include <unordered_map>
#include <unordered_set>


struct Workspace {
    std::string title, version, author, compiler = "c++", linker;
    int standard = 23;
    std::unordered_map<std::string, std::string> vars;

    struct CompileCommand {
        std::string directory, command, file, output;
        std::string abs_output() const;
    };

    struct Target {
        std::string type = "interface";
        std::string base_path;
        std::vector<std::string> sources;
        std::vector<std::string> public_flags;
        std::vector<std::string> public_include_dirs;
        std::vector<std::string> public_depends_on;
        std::vector<std::string> private_flags;
        std::vector<std::string> private_include_dirs;
        std::vector<std::string> private_depends_on;
        std::vector<std::string> link_flags;
        std::vector<CompileCommand> compile_commands;

        bool is_interface() const { return type == "interface"; }
    };

    std::unordered_map<std::string, Target> targets;

    static auto parse(std::string root_dir = "") -> Workspace;
    // TODO: create populate function to only populate git and archive
    void configure(int threads) const;
    void build(const std::string &target, const std::string &out) const;
    void clear(const std::string &target) const;
    void generate_compile_commands_json() const;
    void print_info() const;

protected:
    std::string root_dir, cppxx_cache;

    auto expand_variables(const std::string &input) const -> std::string;
    static auto expand_path(const std::string &pattern) -> std::vector<std::string>;

    void assign_int(const toml::node &node, const std::string &key, int &value) const;
    void assign_string(const toml::node &node, const std::string &key, std::string &value) const;
    void assign_list(const toml::node &node,
                     const std::string &key,
                     std::vector<std::string> &values,
                     std::vector<std::string> *public_values = nullptr,
                     std::vector<std::string> *private_values = nullptr);
    void assign_map(const toml::node &node, const std::string &key, std::unordered_map<std::string, std::string> &map);

    auto populate_git(const toml::node &node, const std::string &key) -> std::string;
    auto populate_archive(const std::string& archive) -> std::string;
    void populate_compile_commands();
    void resolve_dependencies();

    static auto encrypt(const std::string &input, size_t length = 10) -> std::string;

public:
    static int exec(const std::string &file, bool generate_compile_commands);
};
