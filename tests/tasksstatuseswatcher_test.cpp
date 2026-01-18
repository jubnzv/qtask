#include "qt_base_test.hpp"
#include "task.hpp"
#include "tasksstatuseswatcher.hpp"

#include <chrono> //NOLINT
#include <functional>
#include <utility>

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QList>
#include <QSignalSpy>

namespace Test
{
using namespace std::chrono_literals;

class TasksStatusesWatcherTest : public QtBaseTest {
  public:
    struct TasksProvider {
        QList<DetailedTaskInfo> tasks;
        const QList<DetailedTaskInfo> &operator()() const { return tasks; }
        void addTask(QDateTime due, QDateTime sched, QDateTime wait)
        {
            DetailedTaskInfo task;
            task.due = due;
            task.sched = sched;
            task.wait = wait;
            tasks.append(std::move(task));
        }
    };
    TasksProvider tasksProvider;
};

TEST_F(TasksStatusesWatcherTest, InitialStateDoesNotTriggerSignal)
{
    tasksProvider.addTask(QDateTime::currentDateTime().addDays(1), {}, {});
    TasksStatusesWatcher watcher(std::ref(tasksProvider), nullptr,
                                 200ms); // NOLINT
    QSignalSpy spy(&watcher, &TasksStatusesWatcher::statusesWereChanged);

    watcher.startWatchingStatusesChange();

    EXPECT_FALSE(spy.wait(1000)) << "Watcher should not fire signal if "
                                    "statuses haven't changed since start.";
    EXPECT_EQ(spy.count(), 0) << "We do not expect signals when calling "
                                 "startWatchingStatusesChange().";
}

TEST_F(TasksStatusesWatcherTest, TriggersSignalWhenTaskBecomesPast)
{
    constexpr int kFutureDiffSec = 1;
    const auto addTask = [this]() {
        QDateTime future = QDateTime::currentDateTime().addSecs(kFutureDiffSec);
        tasksProvider.addTask(std::move(future), {}, {});
    };

    TasksStatusesWatcher watcher(std::ref(tasksProvider), nullptr, 200ms);
    QSignalSpy spy(&watcher, &TasksStatusesWatcher::statusesWereChanged);

    addTask();
    watcher.startWatchingStatusesChange();

    EXPECT_TRUE(spy.wait(1000 * (kFutureDiffSec + 1)));
    EXPECT_EQ(spy.count(), 1)
        << "It had to be signal that status changed because real time passed.";
    EXPECT_FALSE(spy.wait(1000 * (kFutureDiffSec + 1)));
    EXPECT_EQ(spy.count(), 1) << "Timer had to stop after signal fired.";

    watcher.startWatchingStatusesChange();
    EXPECT_FALSE(spy.wait(1000 * (kFutureDiffSec + 1)));
    EXPECT_EQ(spy.count(), 1)
        << "We do not expect signal as it was no changes.";

    addTask();
    EXPECT_TRUE(spy.wait(1000 * (kFutureDiffSec + 1)));
    EXPECT_GT(spy.count(), 1) << "It had to be signal(s) that status changed.";
}

} // namespace Test
