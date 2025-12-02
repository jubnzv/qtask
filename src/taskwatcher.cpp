#include "taskwatcher.hpp"

#include <QDir>
#include <QFile>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <memory>

namespace
{
///  @brief How often at most we will read new data.
constexpr int kDebounceTimerPeriodMs = 10000;
} // namespace

TaskWatcher::TaskWatcher(QObject *parent)
    : QObject(parent)
{
    m_debounce_timer.setSingleShot(true);
    m_debounce_timer.setInterval(kDebounceTimerPeriodMs);

    connect(&m_debounce_timer, &QTimer::timeout, this,
            [this]() { emit dataOnDiskWereChanged(); });
}

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
            [this](const auto &) { m_debounce_timer.start(); });
    return true;
}
