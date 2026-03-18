#include <gtest/gtest.h>

#include "insight/insight.h"

namespace {

class ArchiveTest : public ::testing::Test {
protected:
    insight::BinaryWriter writer;
};

TEST_F(ArchiveTest, WriteAndReadInt32) {
    int32_t value = 42;
    writer << value;

    insight::BinaryReader reader(writer.GetBuffer());
    int32_t result = 0;
    reader << result;

    EXPECT_EQ(value, result);
}

TEST_F(ArchiveTest, WriteAndReadString) {
    std::string value = "hello";
    writer << value;

    insight::BinaryReader reader(writer.GetBuffer());
    std::string result;
    reader << result;

    EXPECT_EQ(value, result);
}

TEST_F(ArchiveTest, WriteAndReadVector) {
    std::vector<int32_t> value = { 1, 2, 3, 4, 5 };
    writer << value;

    insight::BinaryReader reader(writer.GetBuffer());
    std::vector<int32_t> result;
    reader << result;

    EXPECT_EQ(value, result);
}

TEST_F(ArchiveTest, WriteAndReadMultiple) {
    int32_t     a = 10;
    std::string b = "world";
    int32_t     c = 20;
    writer << a << b << c;

    insight::BinaryReader reader(writer.GetBuffer());
    int32_t     ra = 0, rc = 0;
    std::string rb;
    reader << ra << rb << rc;

    EXPECT_EQ(a, ra);
    EXPECT_EQ(b, rb);
    EXPECT_EQ(c, rc);
}

} // namespace