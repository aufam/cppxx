#include <gtest/gtest.h>
#include <cppxx/iterator.h>

using namespace cppxx;

TEST(iterator, iter) {
    const auto v = std::vector{1, 2, 3};
    int i = 1;
    for (int j : v | iter()) {
        EXPECT_EQ(i, j);
        ++i;
    }
}

TEST(iterator, drop) {
    const std::vector v = {1, 2, 3};
    for (int i : v | drop(2)) {
        EXPECT_EQ(i, 3);
    }
}

TEST(iterator, take) {
    const std::vector v = {1, 2, 3};
    for (int i : v | take(1)) {
        EXPECT_EQ(i, 1);
    }
}

TEST(iterator, step_by) {
    const std::vector v = {1, 2, 3};
    for (int i : v | drop(1) | step_by(2)) {
        EXPECT_EQ(i, 2);
    }
}

TEST(iterator, range) {
    int v = 0;
    for (int i : range(3)) {
        EXPECT_EQ(v, i);
        ++v;
    }

    for (int i : range(3, 6)) {
        EXPECT_EQ(v, i);
        ++v;
    }

    EXPECT_EQ(v, 6);
}

TEST(iterator, enumerate) {
    const std::vector v{1, 2, 3};
    for (auto [i, j] : v | enumerate(1)) {
        EXPECT_EQ(i, j);
    }
}

TEST(iterator, map) {
    for (auto [str, i] : range(10) | map([](int i) { return std::make_pair(std::to_string(i), i); })) {
        EXPECT_EQ(str, std::to_string(i));
    }
}

TEST(iterator, filter) {
    int i = 0;
    for (auto j : range(10) | filter([](int i) { return i % 2 == 0; })) {
        EXPECT_EQ(i, j);
        i += 2;
    }
}

TEST(iterator, map_filter) {
    int i = 0;
    for (auto j : range(10) | map_filter([](int i) -> std::optional<int> {
                      if (i % 2 == 0)
                          return i;
                      return std::nullopt;
                  })) {
        EXPECT_EQ(i, j);
        i += 2;
    }
}
