#ifndef TASKWATCHER_HPP
#define TASKWATCHER_HPP

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

/// @brief This object watches changes of the TaskWarrior specifiec data files
/// on filesystem and triggers full re-read if those were changed.
class TaskWatcher : public QObject {
    Q_OBJECT

  public:
    explicit TaskWatcher(QObject *parent = nullptr);

    bool setup(const QString &task_data_path);
    [[nodiscard]] bool isActive() const
    {
        return m_task_data_watcher != nullptr;
    }

  signals:
    void dataOnDiskWereChanged();

  private:
    std::unique_ptr<QFileSystemWatcher> m_task_data_watcher{ nullptr };
    // Do not react too often.
    QTimer m_debounce_timer;
};

#endif // TASKWATCHER_HPP
