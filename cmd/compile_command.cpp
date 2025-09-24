#include <filesystem>
#include "compile_command.h"

namespace fs = std::filesystem;


std::string CompileCommand::get_abs_path() const {
    fs::path out;

    if (fs::path output = this->output; output.is_relative())
        out = fs::path(this->directory) / output;
    else
        out = output;

    return out.string();
}

