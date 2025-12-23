#include "task_date_time.hpp"

#include <QDateTime>
#include <QString>
#include <qnamespace.h>

#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>

namespace Test
{
struct TaskDateTimeTestCase {
    ETaskDateTimeRole role;
    QString prefix;
};

class TaskDateTimeTest : public ::testing::TestWithParam<TaskDateTimeTestCase> {
};

using namespace std::chrono_literals;

TEST_F(TaskDateTimeTest, Equality)
{
    const TaskDateTime<ETaskDateTimeRole::Due> empty;
    const TaskDateTime<ETaskDateTimeRole::Due> dt1{ QDateTime(
        QDate(2025, 12, 22), QTime(12, 0)) };
    const TaskDateTime<ETaskDateTimeRole::Due> dt2{ QDateTime(
        QDate(2025, 12, 23), QTime(12, 0)) };

    const TaskDateTime<ETaskDateTimeRole::Due> another_empty;
    EXPECT_TRUE(empty == another_empty);

    EXPECT_FALSE(empty == dt1);
    EXPECT_TRUE(empty != dt1);

    EXPECT_FALSE(dt1 == dt2);
    EXPECT_TRUE(dt1 != dt2);

    const TaskDateTime<ETaskDateTimeRole::Due> copy_dt1{ dt1 };
    EXPECT_TRUE(dt1 == copy_dt1);
    EXPECT_FALSE(dt1 != copy_dt1);
}

TEST_P(TaskDateTimeTest, EmptyValueFormat)
{
    const auto [role, prefix] = GetParam();

    switch (role) {
    case ETaskDateTimeRole::Sched: {
        TaskDateTime<ETaskDateTimeRole::Sched> dt;
        EXPECT_EQ(TaskDateTime<ETaskDateTimeRole::Sched>::role_name_cmd(),
                  QString(prefix));
        EXPECT_EQ(dt.relationToNow(), DatesRelation::Future);
        EXPECT_FALSE(dt.has_value());
        EXPECT_THROW(dt.value(), std::logic_error);
        break;
    }
    case ETaskDateTimeRole::Due: {
        EXPECT_EQ(TaskDateTime<ETaskDateTimeRole::Due>::role_name_cmd(),
                  QString(prefix));
        break;
    }
    case ETaskDateTimeRole::Wait: {
        EXPECT_EQ(TaskDateTime<ETaskDateTimeRole::Wait>::role_name_cmd(),
                  QString(prefix));
        break;
    }
    }
}

TEST_P(TaskDateTimeTest, NonEmptyValueFormatAndRelation)
{
    const auto [role, prefix] = GetParam();
    const QDateTime now = QDateTime::currentDateTime();

    switch (role) {
    case ETaskDateTimeRole::Sched: {
        const TaskDateTime<ETaskDateTimeRole::Sched> dt(now.addSecs(10 * 60));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Approaching);
        break;
    }
    case ETaskDateTimeRole::Due: {
        const TaskDateTime<ETaskDateTimeRole::Due> dt(now.addDays(2));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Future);
        break;
    }
    case ETaskDateTimeRole::Wait: {
        const TaskDateTime<ETaskDateTimeRole::Wait> dt(now.addSecs(0));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Approaching);
        break;
    }
    }
}

TEST_P(TaskDateTimeTest, AssignmentOperator)
{
    const auto [role, prefix] = GetParam();
    const QDateTime now = QDateTime::currentDateTime();

    switch (role) {
    case ETaskDateTimeRole::Sched: {
        TaskDateTime<ETaskDateTimeRole::Sched> dt;
        dt = now;
        EXPECT_TRUE(dt.has_value());
        EXPECT_EQ(*dt, now);

        TaskDateTime<ETaskDateTimeRole::Sched> dt2;
        dt2 = dt;
        EXPECT_TRUE(dt2.has_value());
        EXPECT_EQ(*dt2, now);

        TaskDateTime<ETaskDateTimeRole::Sched> dt3;
        dt3 = std::move(dt2);
        EXPECT_TRUE(dt3.has_value());
        EXPECT_EQ(*dt3, now);
        break;
    }
    case ETaskDateTimeRole::Due: {
        TaskDateTime<ETaskDateTimeRole::Due> dt;
        dt = now;
        EXPECT_TRUE(dt.has_value());
        EXPECT_EQ(*dt, now);
        break;
    }
    case ETaskDateTimeRole::Wait: {
        TaskDateTime<ETaskDateTimeRole::Wait> dt;
        dt = now;
        EXPECT_TRUE(dt.has_value());
        EXPECT_EQ(*dt, now);
        break;
    }
    }
}

INSTANTIATE_TEST_SUITE_P(
    TaskDateTimeTests, TaskDateTimeTest,
    ::testing::Values(TaskDateTimeTestCase{ ETaskDateTimeRole::Sched, "sched" },
                      TaskDateTimeTestCase{ ETaskDateTimeRole::Due, "due" },
                      TaskDateTimeTestCase{ ETaskDateTimeRole::Wait, "wait" }));

} // namespace Test
