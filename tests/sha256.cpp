#include <sha256.h>
#include <gtest/gtest.h>


TEST(sha256, sha256) {
    std::string input = "this is a test input";
    std::string hash = SHA256::hashString(input);

    EXPECT_EQ(hash, "bb913d9aa5d53eddb4d2deaf3763c28d864837370904b348c989658a8914bad4");
}
