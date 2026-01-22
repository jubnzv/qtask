#ifndef TASKWATCHER_HPP
#define TASKWATCHER_HPP

#include <QObject>
#include <QTimer>

#include <memory>

#include "pereodic_async_executor.hpp"
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
    void dataOnDiskWereChanged();

  public slots:
    void checkNow();

  private:
    TaskWarriorDbState m_latestDbState;
    std::unique_ptr<IPereodicExec> m_pereodic_worker;
    QTimer delayedSignalSender;
};

#endif // TASKWATCHER_HPP
