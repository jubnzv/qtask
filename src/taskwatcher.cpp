#include "taskwatcher.hpp"

#include <QFile>
#include <QFileSystemWatcher>
#include <QString>

TaskWatcher::TaskWatcher(QObject *parent)
    : m_task_data_path("")
    , m_active(false)
    , m_task_data_watcher(nullptr)
{
}

TaskWatcher::~TaskWatcher() {}

bool TaskWatcher::setup(const QString &task_data_path)
{
    QStringList watch_files = { task_data_path + "pending.data" };
    for (auto const &f : watch_files)
        if (!QFile::exists(f))
            return false;

    m_task_data_watcher = new QFileSystemWatcher(watch_files);
    connect(m_task_data_watcher, &QFileSystemWatcher::fileChanged, this,
            &TaskWatcher::dataChanged);

    m_active = true;
    return true;
}
