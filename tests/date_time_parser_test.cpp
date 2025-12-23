#include "date_time_parser.hpp"

#include <gtest/gtest.h>
#include <QString>
#include <QStringList>

namespace Test
{

using DateTimeParserTest = ::testing::Test;

TEST_F(DateTimeParserTest, SplitWorks)
{
    const QString lexems = "   w1   w2     w3 w4 ";
    const QStringList expected = { "w1", "w2", "w3", "w4" };
    const auto strs = DateTimeParser::splitSpaceSeparatedString(lexems);
    EXPECT_EQ(strs, expected);
}

TEST_F(DateTimeParserTest, ParseWorks)
{
    const QString lexems = "key 2025-11-29 11:58 some more.";
    const DateTimeParser parser{ 5, 1, 2 };
    const auto dt = parser.parseDateTimeString<ETaskDateTimeRole::Due>(lexems);
    ASSERT_TRUE(dt.has_value());
    ASSERT_TRUE(dt->isValid()); // NOLINT

    const auto date = dt->date(); // NOLINT
    EXPECT_EQ(date.year(), 2025);
    EXPECT_EQ(date.month(), 11);
    EXPECT_EQ(date.day(), 29);

    const auto time = dt->time(); // NOLINT
    EXPECT_EQ(time.hour(), 11);
    EXPECT_EQ(time.minute(), 58);
}

TEST_F(DateTimeParserTest, InvalidLenDetected)
{
    const QString lexems = "key 2025-11-29 11:58 some more.";
    EXPECT_FALSE(
        (DateTimeParser{ 7, 1, 2 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 4, 1, 2 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 2, 1, 2 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
}

TEST_F(DateTimeParserTest, InvalidIndexesGiven)
{
    const QString lexems = "key 2025-11-29 11:58 some more.";
    EXPECT_FALSE(
        (DateTimeParser{ 5, 1, 3 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 5, 2, 1 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 5, 2, 2 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 5, 1, 4 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
    EXPECT_FALSE(
        (DateTimeParser{ 5, 3, 4 }.parseDateTimeString<ETaskDateTimeRole::Due>(
             lexems))
            .has_value());
}

} // namespace Test
