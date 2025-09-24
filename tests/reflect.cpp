#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rfl/toml.hpp>
#include <fmt/ranges.h>
#include <gtest/gtest.h>


TEST(reflect, reflect) {
    enum class Color : uint8_t { red, yellow, black };
    struct Person {
        rfl::Rename<"firstName", std::string> first_name;
        rfl::Rename<"lastName", std::string> last_name;
        int age;
        Color skin;
        std::unordered_map<std::string, std::string> map;
        std::optional<std::vector<uint8_t>> bytes;
    };

    const auto homer = Person{
        .first_name = "Homer",
        .last_name = "Simpson",
        .age = 45,
        .skin = Color::yellow,
        .map = {{"test", "!@#"}},
        .bytes = std::vector<uint8_t>{1, 2, 3}
    };

    std::string json_string = rfl::json::write(homer);
    fmt::println(stderr, "refl json {:?}", json_string);

    json_string = R"({"firstName":"Homer","lastName":"Simpson","age":45,"skin":"yellow","map":{"test":"!@#"}})";
    auto as = rfl::json::read<Person>(json_string);

    if (as) {
        auto toml_string = rfl::toml::write(as.value());
        fmt::println(stderr, "refl toml: \n{}", toml_string);
    } else {
        fmt::println(stderr, "err = {:?}", as.error().what());
    }
}
