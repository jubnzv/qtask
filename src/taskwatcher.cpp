#include "taskwatcher.hpp"

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QString>

TaskWatcher::TaskWatcher(QObject *parent) {}

TaskWatcher::~TaskWatcher() = default;

bool TaskWatcher::setup(const QString &task_data_path)
{
    const auto addPath = [&task_data_path](const QString &fileName) {
        return task_data_path + QDir::separator() + fileName;
    };

    const QStringList fileNamesCandidates = {
        addPath("pending.data"),
        addPath("taskchampion.sqlite3"),
    };

    const auto it =
        std::find_if(fileNamesCandidates.begin(), fileNamesCandidates.end(),
                     [](const auto &p) { return QFile::exists(p); });

    if (it == fileNamesCandidates.end()) {
        return false;
    }

    m_task_data_watcher.reset(new QFileSystemWatcher({ *it }));
    connect(m_task_data_watcher.get(), &QFileSystemWatcher::fileChanged, this,
            &TaskWatcher::dataChanged);
    return true;
}
