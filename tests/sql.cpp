#include <fmt/ranges.h>
#include <cppxx/sql.h>
#include <gtest/gtest.h>

using namespace cppxx;

struct User {
    // clang-format off
    int id;           using Id   = sql::Column<int,         "id integer primary key">;
    std::string name; using Name = sql::Column<std::string, "name varchar(32) not null">;
    int age;          using Age  = sql::Column<int,         "age integer">;
    // clang-format on

    using Schema = sql::Schema<"Users", Id, Name, Age>;
};

TEST(sql, sql) {
    EXPECT_EQ(User::Id::name, literal("id"));
    EXPECT_EQ(User::Id::sql_type, literal("integer"));
    EXPECT_EQ(User::Id::sql_constraint, literal("primary key"));

    EXPECT_EQ(User::Name::name, literal("name"));
    EXPECT_EQ(User::Name::sql_type, literal("varchar(32)"));
    EXPECT_EQ(User::Name::sql_constraint, literal("not null"));

    EXPECT_EQ(User::Age::name, literal("age"));
    EXPECT_EQ(User::Age::sql_type, literal("integer"));
    EXPECT_EQ(User::Age::sql_constraint, literal(""));

    auto expect_eq_stmt = [](auto &&stmt, auto &&value, auto &&params) {
        fmt::println("stmt={:?}", stmt.stmt);
        fmt::println("params={}", params);
        EXPECT_EQ(stmt.stmt, value);
        EXPECT_EQ(stmt.params, params);
    };

    expect_eq_stmt(sql::create_table<User>,
                   literal("create table Users (id integer primary key, name varchar(32) not null, age integer)"),
                   std::tuple{});

    expect_eq_stmt(sql::insert_into<User>(User::Id{}, User::Name{}, User::Age{}).values({10, "Sucipto", 30}),
                   literal("insert into Users (id, name, age) values (?, ?, ?)"),
                   std::tuple{10, "Sucipto", 30});

    expect_eq_stmt(
        sql::update<User>.set(User::Name{} = "Tejo", User::Age{} = 20).where(User::Name{} == "Sucipto" or User::Age{} > 10 and User::Id{} >= 10),
        literal("update Users set name = ?, age = ? where (name = ? or (age > ? and id >= ?))"),
        std::tuple{"Tejo", 20, "Sucipto", 10, 10});

    expect_eq_stmt(
        sql::select<User::Id>.from(User{}).where(User::Age{} > 10), literal("select id from Users where age > ?"), std::tuple{10});

    // {
    //     auto stmt = sql::Statement<"update Users">{}
    //                     .set(User::Name{} = "Tejo", User::Age{} = 20)
    //                     .where(User::Name{} == "Sucipto" or User::Age{} > 10 and User::Id{} >= 10);
    //     fmt::println("stmt = {}", stmt);
    // }
    // {
    //     auto stmt = insert_into<User>(User::Id{}, User::Name{}, User::Age{}).values({10, "Sucipto", 30});
    //     fmt::println("stmt = {:?}", stmt.value);
    //     fmt::println("placeholders = {}", stmt.placeholders);
    //     fmt::println("values = {}", typeid(decltype(stmt)::values_type).name());
    // }
    // {
    //     auto stmt = sqlselect<User::Id, User::Name>.from(User{});
    //
    //     fmt::println("stmt = {:?}", stmt.value);
    //     fmt::println("placeholders = {}", stmt.placeholders);
    //     fmt::println("values = {}", stmt.gettest());
    // }
}
