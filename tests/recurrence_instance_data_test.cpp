#include "recurrence_instance_data.hpp"

#include <gtest/gtest.h>

#include <optional>

namespace Test
{
class RecurrentInstancePeriodTest : public ::testing::Test {};

TEST_F(RecurrentInstancePeriodTest, InitialStateIsUnknown)
{
    const RecurrentInstancePeriod p;

    EXPECT_FALSE(p.isRead());
    EXPECT_FALSE(p.isRecurrent());
    EXPECT_FALSE(p.isFullyRead());
    EXPECT_EQ(p.period(), std::nullopt);
}

TEST_F(RecurrentInstancePeriodTest, NonReccurentState)
{
    RecurrentInstancePeriod p;
    p.setNonRecurrent();

    EXPECT_TRUE(p.isRead());
    EXPECT_FALSE(p.isRecurrent());
    EXPECT_TRUE(p.isFullyRead());
    EXPECT_EQ(p.period(), std::nullopt);
}

TEST_F(RecurrentInstancePeriodTest, ReccurentButMissingPeriod)
{
    RecurrentInstancePeriod p;
    p.setRecurrent();

    EXPECT_TRUE(p.isRead());
    EXPECT_TRUE(p.isRecurrent());
    EXPECT_FALSE(p.isFullyRead());
    EXPECT_EQ(p.period(), std::nullopt);
}

TEST_F(RecurrentInstancePeriodTest, FullReccurenceWithPeriod)
{
    RecurrentInstancePeriod p;
    const QString val = "1w";
    p.setRecurrent(val);

    EXPECT_TRUE(p.isRead());
    EXPECT_TRUE(p.isRecurrent());
    EXPECT_TRUE(p.isFullyRead());

    ASSERT_TRUE(p.period().has_value());
    EXPECT_EQ(p.period().value(), val); // NOLINT
}

TEST_F(RecurrentInstancePeriodTest, EqualityOperators)
{
    RecurrentInstancePeriod p1, p2;

    EXPECT_TRUE(p1 == p2);

    p1.setNonRecurrent();
    EXPECT_TRUE(p1 != p2);

    p2.setNonRecurrent();
    EXPECT_TRUE(p1 == p2);

    p1.setRecurrent("daily");
    p2.setRecurrent("weekly");
    EXPECT_FALSE(p1 == p2);

    p2.setRecurrent("daily");
    EXPECT_TRUE(p1 == p2);
}

TEST_F(RecurrentInstancePeriodTest, StateTransition)
{
    RecurrentInstancePeriod p;

    p.setRecurrent();
    EXPECT_FALSE(p.isFullyRead());
    EXPECT_EQ(p.period(), std::nullopt);

    p.setRecurrent("1m");
    EXPECT_TRUE(p.isFullyRead());
    ASSERT_TRUE(p.period().has_value());
    EXPECT_EQ(p.period().value(), "1m"); // NOLINT

    p.setNonRecurrent();
    EXPECT_FALSE(p.isRecurrent());
    EXPECT_EQ(p.period(), std::nullopt);
}
} // namespace Test
