#ifndef TASKWATCHER_HPP
#define TASKWATCHER_HPP

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

class TaskWatcher : public QObject {
    Q_OBJECT

  public:
    TaskWatcher(QObject *parent = nullptr);
    ~TaskWatcher();

    bool setup(const QString &task_data_path);
    bool isActive() const { return m_active; }

  signals:
    void dataChanged(const QString &);

  private:
    QString m_task_data_path;
    bool m_active;
    QFileSystemWatcher *m_task_data_watcher;
};

#endif // TASKWATCHER_HPP
