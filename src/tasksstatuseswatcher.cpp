#include "tasksstatuseswatcher.hpp"
#include "task_date_time.hpp"

#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <qtmetamacros.h>

#include <chrono>
#include <stdexcept>
#include <utility>
namespace
{

DatesRelation &relationForRole(TasksStatusesWatcher::StatusPerTask &status,
                               const ETaskDateTimeRole role) noexcept(false)
{
    switch (role) {
    case ETaskDateTimeRole::Due:
        return std::get<0>(status);
    case ETaskDateTimeRole::Sched:
        return std::get<1>(status);
    case ETaskDateTimeRole::Wait:
        return std::get<2>(status);
    }
    throw std::runtime_error("Unhandled role.");
}

constexpr const DatesRelation &
relationForRole(const TasksStatusesWatcher::StatusPerTask &status,
                const ETaskDateTimeRole role) noexcept(false)
{
    switch (role) {
    case ETaskDateTimeRole::Due:
        return std::get<0>(status);
    case ETaskDateTimeRole::Sched:
        return std::get<1>(status);
    case ETaskDateTimeRole::Wait:
        return std::get<2>(status);
    }
    throw std::runtime_error("Unhandled role.");
}

TasksStatusesWatcher::Statuses
computeStatusesNow(const QList<DetailedTaskInfo> &tasks)
{
    const QDateTime now = QDateTime::currentDateTime();

    TasksStatusesWatcher::Statuses currentStatuses;
    currentStatuses.reserve(tasks.size());
    for (const auto &task : tasks) {
        TasksStatusesWatcher::StatusPerTask taskStatus;
        relationForRole(taskStatus, ETaskDateTimeRole::Due) =
            task.due.get().relationToNow(now);
        relationForRole(taskStatus, ETaskDateTimeRole::Sched) =
            task.sched.get().relationToNow(now);
        relationForRole(taskStatus, ETaskDateTimeRole::Wait) =
            task.wait.get().relationToNow(now);

        currentStatuses.emplace_back(std::move(taskStatus));
    }
    return currentStatuses;
}

} // namespace

TasksStatusesWatcher::TasksStatusesWatcher(
    TasksProvider tasksProvider, QObject *parent,
    std::chrono::milliseconds checkInterval)
    : QObject{ parent }
    , tasks_provider(std::move(tasksProvider))
{
    periodic_statuses_check.setSingleShot(true);
    periodic_statuses_check.setInterval(
        static_cast<int>(checkInterval.count()));

    QTimer::connect(&periodic_statuses_check, &QTimer::timeout, this,
                    &TasksStatusesWatcher::checkStatusesNow);
}

void TasksStatusesWatcher::startWatchingStatusesChange()
{
    last_known_statuses = computeStatusesNow(tasks_provider());
    periodic_statuses_check.start();
}

void TasksStatusesWatcher::checkStatusesNow()
{
    auto currentStatuses = computeStatusesNow(tasks_provider());
    if (currentStatuses != last_known_statuses) {
        last_known_statuses = std::move(currentStatuses);
        emit statusesWereChanged();
        return;
    }
    periodic_statuses_check.start();
}
