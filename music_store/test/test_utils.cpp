#include "utils.h"
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <cstdio>

TEST(UtilsTest, TrimRemovesWhitespace) {
    EXPECT_EQ(utils::trim("  hello world  "), "hello world");
    EXPECT_EQ(utils::trim("\t  test\n"), "test");
    EXPECT_EQ(utils::trim("noSpaces"), "noSpaces");
}

TEST(UtilsTest, GetCurrentDateFormat) {
    std::string date = utils::get_current_date();
    EXPECT_EQ(date.size(), 10);
    EXPECT_EQ(date[4], '-');
    EXPECT_EQ(date[7], '-');
}

TEST(UtilsTest, CopyFileWorks) {
    std::string src = "test_src.txt";
    std::string dst = "test_dst.txt";
    {
        std::ofstream out(src);
        out << "Hello, copy!";
    }
    EXPECT_TRUE(utils::copy_file(src, dst));
    std::ifstream in(dst);
    std::string content;
    std::getline(in, content);
    EXPECT_EQ(content, "Hello, copy!");
    std::remove(src.c_str());
    std::remove(dst.c_str());
}

TEST(UtilsTest, Sha256ReturnsInput) {
    EXPECT_EQ(utils::sha256("admin"), "admin");
}
