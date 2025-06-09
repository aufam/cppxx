#include <string>
#include <vector>
#include <variant>
#include <optional>


struct Option {
    template <typename T>
    Option &add_option(T &,
                       std::string key,
                       std::string help,
                       std::optional<std::string> default_value = std::nullopt,
                       bool is_positional = false);

    template <typename T>
    Option &add_option(T &,
                       char key_char,
                       std::string key_str,
                       std::string help,
                       std::optional<std::string> default_value = std::nullopt,
                       bool is_positional = false);

    std::string parse(const std::string &app_name, int argc, char **argv);

private:
    struct Cache {
        std::variant<int *,
                     float *,
                     bool *,
                     std::string *,
                     std::vector<int> *,
                     std::vector<float> *,
                     std::vector<bool> *,
                     std::vector<std::string> *>
            target;
        char key_char;
        std::string key_str, help;
        std::optional<std::string> default_value;
        bool is_positional;
    };
    std::vector<Cache> caches;
};
