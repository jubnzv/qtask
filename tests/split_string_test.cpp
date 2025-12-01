#include "split_string.hpp"

#include <gtest/gtest.h>

namespace Test
{
using SplitStringTest = ::testing::Test;

TEST_F(SplitStringTest, CanSplit)
{
    const SplitString split("key value and more value   ");
    EXPECT_EQ(split.key, QString("key"));
    EXPECT_EQ(split.value, QString("value and more value"));
    EXPECT_TRUE(split.isValid());
}

TEST_F(SplitStringTest, KeyStartsAtStringStart)
{
    const SplitString split("      key value and more value  ");
    EXPECT_EQ(split.key, QString(""));
    EXPECT_EQ(split.value, QString(""));
    EXPECT_FALSE(split.isValid());
}

} // namespace Test
