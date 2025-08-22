#include <fmt/ranges.h>
#include <json.hpp>
#include <cppxx/json.h>
#include <gtest/gtest.h>

using namespace cppxx;

struct User {
    int id;
    std::string name;
    int age;

    struct Id : json::Field<User, "id"> {
        int &ref(User &user) const { return user.id; }
        const int &ref(const User &user) const { return user.id; }
    };

    struct Name : json::Field<User, "name"> {
        std::string &ref(User &user) const { return user.name; }
        const std::string &ref(const User &user) const { return user.name; }
    };

    struct Age : json::Field<User, "age"> {
        int &ref(User &user) const { return user.age; }
        const int &ref(const User &user) const { return user.age; }
    };

    template <typename J>
    using const_JSON = json::const_Ref<J, User, Id, Name, Age>;

    template <typename J>
    using JSON = json::Ref<J, User, Id, Name, Age>;
};

inline void to_json(nlohmann::json &j, const User &user) { cppxx::json::to_json(j, User::const_JSON<nlohmann::json>{user}); }

inline void from_json(const nlohmann::json &j, User &user) { cppxx::json::from_json(j, User::JSON<nlohmann::json>{user}); }

TEST(json, json) {
    User user = {.id = 2, .name = "Sucipto", .age = 20};

    nlohmann::json js = user;
    fmt::println("[DEBUG] json serialize = {:?}", js.dump());

    js["name"] = "Sugeng";
    user = js;

    fmt::println("[DEBUG] json deserialize = {}, {}, {}", user.id, user.name, user.age);
}
