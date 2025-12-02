#ifndef TASKWATCHER_HPP
#define TASKWATCHER_HPP

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>

#include <atomic>

#include "task.hpp"

/// @brief This object polls TaskWarrior and fires signal when re-read data is
/// needed.
/// @note It is initialized in untracking state. You must call checkNow() at
/// least once.
class TaskWatcher : public QObject {
    Q_OBJECT

  public:
    explicit TaskWatcher(QObject *parent = nullptr);

  signals:
    // FIXME/TODO: what will happen when we will have 100'000 records in DB?
    void dataOnDiskWereChanged();

  public slots:
    void checkNow();

  private:
    QFutureWatcher<TaskWarriorDbState::Optional> *m_state_reader;
    QTimer m_check_for_changes_timer;
    TaskWarriorDbState m_latestDbState;
    std::atomic<bool> m_slot_once{ false };
};

#endif // TASKWATCHER_HPP
