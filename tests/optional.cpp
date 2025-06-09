#include <gtest/gtest.h>
#include <cppxx/iterator.h>

using namespace cppxx;

TEST(optional, map) {
    std::optional<int> a = 10;
    auto b = a | map([](int a) { return a + 10; });
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(*b, 20);

    auto c = b.and_then([](int b) -> std::optional<int> { return b + 10; });
}

TEST(expected, map) {
    std::expected<int, int> a = 10;
    auto b = a | map([](int a) { return a + 10; });
    EXPECT_TRUE(b.has_value());
    EXPECT_EQ(*b, 20);
}
