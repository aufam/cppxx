#include <fmt/ranges.h>
#include <cppxx/tuple.h>
#include <gtest/gtest.h>
#include <ranges>

using namespace cppxx;

TEST(tuple, tuple_like) {
    EXPECT_TRUE((tuple_like<std::tuple<int>>));
    EXPECT_TRUE((tuple_like<std::pair<int, int>>));
    EXPECT_TRUE((tuple_like<std::array<int, 3>>));
    EXPECT_TRUE((tuple_like<std::ranges::subrange<int*>>));
    EXPECT_FALSE((tuple_like<std::variant<int, std::string>>));
    EXPECT_FALSE((tuple_like<std::optional<int>>));
    EXPECT_FALSE((tuple_like<int>));
}

TEST(tuple, format) {
    EXPECT_EQ(fmt::format("{}", std::make_tuple(1, "string")), "(1, \"string\")");
    EXPECT_EQ(fmt::format("{}", std::make_pair(1, "2")), "(1, \"2\")");
}
