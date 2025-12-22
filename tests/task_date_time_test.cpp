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

class TaskDateTimeParameterizedTest
    : public ::testing::TestWithParam<TaskDateTimeTestCase> {};

using namespace std::chrono_literals;

TEST_P(TaskDateTimeParameterizedTest, EmptyValueFormat)
{
    const auto [role, prefix] = GetParam();

    switch (role) {
    case ETaskDateTimeRole::Sched: {
        TaskDateTime<ETaskDateTimeRole::Sched> dt;
        EXPECT_EQ(task_date_time_format::formatForCmd(dt), QString(prefix));
        EXPECT_EQ(dt.relationToNow(), DatesRelation::Future);
        EXPECT_FALSE(dt.has_value());
        EXPECT_THROW(dt.value(), std::logic_error);
        break;
    }
    case ETaskDateTimeRole::Due: {
        const TaskDateTime<ETaskDateTimeRole::Due> dt;
        EXPECT_EQ(task_date_time_format::formatForCmd(dt), QString(prefix));
        break;
    }
    case ETaskDateTimeRole::Wait: {
        const TaskDateTime<ETaskDateTimeRole::Wait> dt;
        EXPECT_EQ(task_date_time_format::formatForCmd(dt), QString(prefix));
        break;
    }
    }
}

TEST_P(TaskDateTimeParameterizedTest, NonEmptyValueFormatAndRelation)
{
    const auto [role, prefix] = GetParam();
    const QDateTime now = QDateTime::currentDateTime();

    switch (role) {
    case ETaskDateTimeRole::Sched: {
        const TaskDateTime<ETaskDateTimeRole::Sched> dt(now.addSecs(10 * 60));
        EXPECT_EQ(task_date_time_format::formatForCmd(dt),
                  prefix + dt->toString(Qt::ISODate));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Approaching);
        break;
    }
    case ETaskDateTimeRole::Due: {
        const TaskDateTime<ETaskDateTimeRole::Due> dt(now.addDays(2));
        EXPECT_EQ(task_date_time_format::formatForCmd(dt),
                  prefix + dt->toString(Qt::ISODate));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Future);
        break;
    }
    case ETaskDateTimeRole::Wait: {
        const TaskDateTime<ETaskDateTimeRole::Wait> dt(now.addSecs(0));
        EXPECT_EQ(task_date_time_format::formatForCmd(dt),
                  prefix + dt->toString(Qt::ISODate));
        EXPECT_EQ(dt.relationToNow(now), DatesRelation::Approaching);
        break;
    }
    }
}

TEST_P(TaskDateTimeParameterizedTest, AssignmentOperator)
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
    TaskDateTimeTests, TaskDateTimeParameterizedTest,
    ::testing::Values(
        TaskDateTimeTestCase{ ETaskDateTimeRole::Sched, "sched:" },
        TaskDateTimeTestCase{ ETaskDateTimeRole::Due, "due:" },
        TaskDateTimeTestCase{ ETaskDateTimeRole::Wait, "wait:" }));

} // namespace Test
