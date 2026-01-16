#pragma once

#include "task.hpp"
#include "task_date_time.hpp"

#include <chrono>
#include <functional>
#include <tuple>
#include <vector>

#include <QList>
#include <QObject>
#include <QTimer>

//! @brief This class tracks possible status changes and generate signal, which
//! can be used to re-read taskwatcher. Status changes because of natural time
//! pass.
class TasksStatusesWatcher : public QObject {
    Q_OBJECT
  public:
    using StatusPerTask =
        std::tuple<DatesRelation, DatesRelation, DatesRelation>;
    using Statuses = std::vector<StatusPerTask>;

    using TasksProvider = std::function<const QList<DetailedTaskInfo> &()>;
    explicit TasksStatusesWatcher(
        TasksProvider tasksProvider, QObject *parent = nullptr,
        std::chrono::milliseconds checkInterval = std::chrono::minutes(5));

  public slots:
    /// @brief Records statuses now, as base for watching in future. Does not
    /// produce signal statusesWereChanged().
    /// @note It must be called at least once to start watching.
    void startWatchingStatusesChange();
  signals:
    /// @brief Signal is sent when we detect recorded statuses were changed
    /// comparing to what tasks_provider provides right now.
    /// @note Once signal is sent, statuses are no longer watched, it must be
    /// restart by call to startWatchingStatusesChange().
    void statusesWereChanged();

  private slots:
    void checkStatusesNow();

  private:
    TasksProvider tasks_provider;
    Statuses last_known_statuses;
    QTimer periodic_statuses_check;
};
