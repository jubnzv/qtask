#ifndef TASKWATCHER_HPP
#define TASKWATCHER_HPP

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

#include <memory>

class TaskWatcher : public QObject {
    Q_OBJECT

  public:
    explicit TaskWatcher(QObject *parent = nullptr);
    ~TaskWatcher() override;

    bool setup(const QString &task_data_path);
    [[nodiscard]] bool isActive() const
    {
        return m_task_data_watcher != nullptr;
    }

  signals:
    void dataChanged(const QString &);

  private:
    std::unique_ptr<QFileSystemWatcher> m_task_data_watcher{ nullptr };
};

#endif // TASKWATCHER_HPP
