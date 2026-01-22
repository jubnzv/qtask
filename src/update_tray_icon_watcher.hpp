#pragma once

#include "pereodic_async_executor.hpp"
#include "task.hpp"
#include "task_emojies.hpp"

#include <QList>
#include <QObject>

#include <memory>

/// @brief This object queries DB periodically to make slice of the tasks which
/// will become (or already did) "hot" in nearest future
/// (TaskDateTime<>::warning_interval()).
/// Then it check those tasks in memory periodically and fast to detect what
/// icon should we show.
/// @note This list does not use any user set filters.
class UpdateTrayIconWatcher : public QObject {
    Q_OBJECT
  public:
    explicit UpdateTrayIconWatcher(QObject *parent = nullptr);
  signals:
    void globalUrgencyChanged(StatusEmoji::EmojiUrgency);

  public slots:
    void checkNow();

  private:
    /// @brief Queries taskwarrior asynchroniously for "hot tasks".
    std::unique_ptr<IPereodicExec> m_pereodic_worker;
    /// @brief Subset of the tasks which will become "hot" in near future (but
    /// could be not yet) or already missed.
    QList<DetailedTaskInfo> m_hot_tasks;
};
