#include <expected>
#include <string>
#include <unordered_map>
#include <vector>



struct CompileCommands {
    std::string directory, command, file, output;
};
#ifdef NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CompileCommands, directory, command, file, output)
#endif


struct Project {
    std::string type = "interface";
    std::vector<std::string> sources;
    std::vector<std::string> public_flags;
    std::vector<std::string> public_include_dirs;
    std::vector<std::string> public_depends_on;
    std::vector<std::string> private_flags;
    std::vector<std::string> private_include_dirs;
    std::vector<std::string> private_depends_on;
    std::vector<CompileCommands> compile_commands;
    std::string base_path;
};
#ifdef NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Project, type, sources, public_flags, public_include_dirs, public_depends_on, private_flags,
                                   private_include_dirs, private_depends_on)
#endif


struct Workspace {
    std::string title, version, author, compiler = "c++";
    int standard = 23;
    std::unordered_map<std::string, std::string> vars;
    std::unordered_map<std::string, Project> project;
};
#ifdef NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Workspace, title, version, author, compiler, standard, project)
#endif


auto parse_workspace(std::string root_dir = "", std::string mode = "debug") -> Workspace;
