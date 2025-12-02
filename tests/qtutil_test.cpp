#include "exec_on_exit.hpp"
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

TEST_F(QtUtilTest, ExecOnExitWorks)
{
    int val = 0;
    {
        EXPECT_EQ(val, 0);
        const exec_on_exit totest([&val]() { val = 1; });
        EXPECT_EQ(val, 0);
    }
    EXPECT_EQ(val, 1);
}

} // namespace Test
