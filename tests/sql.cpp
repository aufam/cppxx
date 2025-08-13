#include <fmt/ranges.h>
#include <cppxx/sql/sql.h>
#include <gtest/gtest.h>
#include <cxxabi.h>
#include <memory>

std::string pretty_name(const char *name) {
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};
    return (status == 0) ? res.get() : name;
}

using namespace cppxx;

struct User {
    // clang-format off
    int id;           using Id   = sql::Column<int,         "id integer primary key">;
    std::string name; using Name = sql::Column<std::string, "name varchar(32) not null">;
    int age;          using Age  = sql::Column<int,         "age integer">;
    // clang-format on

    using _TableDefinition = sql::TableDefinition<"Users", Id, Name, Age>;
};

template <size_t i>
struct repeated_placeholders {
    static constexpr literal value = repeated_placeholders<i - 1>::value + literal(", ?");
};

template <>
struct repeated_placeholders<0> {
    static constexpr literal value = "";
};

template <>
struct repeated_placeholders<1> {
    static constexpr literal value = "?";
};

template <literal Stmt, tuple_like Placeholders = std::tuple<>, tuple_like Values = std::tuple<>>
struct Statement {
    static constexpr literal value = Stmt;
    using placeholders_type = Placeholders;
    using values_type = Values;

    Placeholders placeholders;

    template <literal OtherStatement, typename OtherPlaceholders, typename OtherValues>
    constexpr auto operator+(const Statement<OtherStatement, OtherPlaceholders, OtherValues> &other) const {
        return Statement<value + OtherStatement,
                         decltype(std::tuple_cat(placeholders, other.placeholders)),
                         decltype(std::tuple_cat(Values{}, OtherValues{}))>{std::tuple_cat(placeholders, other.placeholders)};
    }

    template <typename Other>
    constexpr auto operator&&(const Other &other) const {
        return Statement<"(">{} + *this + Statement<" and ">{} + other + Statement<")">{};
    }

    template <typename Other>
    constexpr auto operator||(const Other &other) const {
        return Statement<"(">{} + *this + Statement<" or ">{} + other + Statement<")">{};
    }

    constexpr auto operator!() const { return Statement<"not (">{} + *this + Statement<")">{}; }

    template <typename Other, typename... Rest>
    constexpr auto set(const Other &other, const Rest &...rest) const {
        return *this + Statement<" set ">{} + other + ((Statement<", ">{} + rest) + ...);
    }

    template <typename Table>
    constexpr auto from(const Table &) const {
        return *this + Statement<literal(" ") + Table::_TableDefinition::name>{};
    }

    template <literal alias>
    constexpr auto as() const {
        return *this + Statement<literal(" as ") + alias>{};
    }

    template <typename Condition>
    constexpr auto where(const Condition &condition) const {
        return *this + Statement<" where ">{} + condition;
    }

    template <typename Col, typename... Cols>
    constexpr auto operator()(const Col &, const Cols &...) const {
        return *this + Statement<literal(" (") + Col::name, std::tuple<typename Col::type>>{}
        + (Statement<literal(", ") + Cols::name, std::tuple<typename Cols::type>>{} + ...) + Statement<")">{};
    }

    constexpr auto values(const Placeholders &p) const {
        return Statement<Stmt + literal(" values (") + repeated_placeholders<std::tuple_size_v<Placeholders>>::value
                             + literal(")"),
                         Placeholders,
                         Values>{p};
    }

    constexpr auto gettest() const { return Values{}; }
};


template <typename Table>
static constexpr auto insert_into = Statement<literal("insert into ") + Table::_TableDefinition::name>{};

template <typename Col, typename... Cols>
static constexpr auto sqlselect = Statement<literal("select ") + Col::name + ((literal(", ") + Cols::name) + ...),
                                            std::tuple<>,
                                            std::tuple<typename Col::type, typename Cols::type...>>{};

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

    auto expect_eq_stmt = [](auto &&stmt, auto &&value, auto &&placeholders) {
        EXPECT_EQ(stmt.value, value);
        EXPECT_EQ(stmt.placeholders, placeholders);
    };

    expect_eq_stmt(sql::create_table<User>,
                   literal("create table Users (id integer primary key, name varchar(32) not null, age integer);"),
                   std::tuple{});

    expect_eq_stmt(sql::insert_into<User, User::Id, User::Name, User::Age>::values(10, "Sucipto", 30),
                   literal("insert into Users (id, name, age) values (?, ?, ?);"),
                   std::tuple{10, "Sucipto", 30});

    expect_eq_stmt(sql::update<User>::set(User::Name{} = "Tejo", User::Age{} = 20)
                       .where(User::Name{} == "Sucipto" or User::Age{} > 10 and User::Id{} >= 10),
                   literal("update Users set name = ?, age = ? where (name = ? or (age > ? and id >= ?));"),
                   std::tuple{"Tejo", 20, "Sucipto", 10, 10});

    {
        auto stmt = sql::Statement<"update Users">{}
                        .set(User::Name{} = "Tejo", User::Age{} = 20)
                        .where(User::Name{} == "Sucipto" or User::Age{} > 10 and User::Id{} >= 10);
        fmt::println("stmt = {}", stmt);
    }
    {
        auto stmt = insert_into<User>(User::Id{}, User::Name{}, User::Age{}).values({10, "Sucipto", 30});
        fmt::println("stmt = {:?}", stmt.value);
        fmt::println("placeholders = {}", stmt.placeholders);
        fmt::println("values = {}", typeid(decltype(stmt)::values_type).name());
    }
    {
        auto stmt = sqlselect<User::Id, User::Name>.from(User{});

        fmt::println("stmt = {:?}", stmt.value);
        fmt::println("placeholders = {}", stmt.placeholders);
        fmt::println("values = {}", stmt.gettest());
    }
}
