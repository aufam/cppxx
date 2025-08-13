#ifndef CPPXX_CLI_OPTION_H
#define CPPXX_CLI_OPTION_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <unordered_set>
#include <exception>


namespace cppxx::cli {
    struct Option {
        std::variant<bool *,
                     int *,
                     float *,
                     std::string *,

                     std::optional<int> *,
                     std::optional<float> *,
                     std::optional<std::string> *,

                     std::vector<int> *,
                     std::vector<float> *,
                     std::vector<std::string> *,

                     std::optional<std::vector<int>> *,
                     std::optional<std::vector<float>> *,
                     std::optional<std::vector<std::string>> *>
            target;
        char key_char = '\0';
        std::string key_str, help;
        bool is_positional = false;
        std::unordered_set<std::variant<int, std::string>> one_of = {};
    };

    void parse(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);
    void parse_or_throw(const std::string &app_name, int argc, char **argv, const std::vector<Option> &options);

    class parse_error : public std::exception {
    public:
        explicit parse_error(std::string msg)
            : msg(std::move(msg)) {}
        ~parse_error() override = default;

        [[nodiscard]] const char *what() const noexcept override { return msg.c_str(); }

    private:
        std::string msg;
    };

    class parse_help : public std::exception {
    public:
        explicit parse_help(std::string msg)
            : msg(std::move(msg)) {}
        ~parse_help() override = default;

        [[nodiscard]] const char *what() const noexcept override { return msg.c_str(); }

    private:
        std::string msg;
    };
} // namespace cppxx::cli


#endif
