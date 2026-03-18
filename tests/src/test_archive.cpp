#include <gtest/gtest.h>

#include "insight/archive.h"

namespace {

TEST(ArchiveTest, WriteAndReadInt) {
    insight::BinaryWriter writer;
    int32_t value = 42;
    writer << value;

    insight::BinaryReader reader(writer.GetBuffer());
    int32_t result = 0;
    reader << result;

    EXPECT_EQ(value, result);
}

} // namespace