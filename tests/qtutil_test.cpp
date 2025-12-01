#include "qtutil.hpp"

#include <gtest/gtest.h>

namespace Test
{
using QtUtilTest = ::testing::Test;

TEST_F(QtUtilTest, IsIntegerWorks)
{
    EXPECT_TRUE(isInteger("232345871"));
    EXPECT_FALSE(isInteger("23SKF2345871"));
    EXPECT_FALSE(isInteger("SDDDDDDD"));
}

} // namespace Test
