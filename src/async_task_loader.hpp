#pragma once

#include <QFuture>
#include <QFutureWatcher>
#include <QMap>
#include <QObject>
#include <QString>
#include <QtConcurrent>

#include "configmanager.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <atomic>
#include <cstddef>
#include <limits>
#include <utility>

/// @brief Loads task asynchroniously.
/// @note It does `task information`.
class AsyncTaskLoader : public QObject {
    Q_OBJECT
  public:
    using RequestId = std::size_t;
    static constexpr auto kInvalidRequestId =
        std::numeric_limits<RequestId>::max();

    static AsyncTaskLoader *create(QObject *parent = nullptr)
    {
        return new AsyncTaskLoader(parent);
    }
    AsyncTaskLoader() = delete;

    template <typename taUserParameters>
    RequestId startTaskLoad(taUserParameters userParameters,
                            const DetailedTaskInfo &partialTask)
    {
        const auto currentRequestId = ++m_latestRequestId;
        auto *watcher = new TaskFutWatcher(this);
        m_activeWatchers.insert(currentRequestId, watcher);

        // Lambda will be called in GUI thread.
        connect(watcher, &QFutureWatcher<DetailedTaskInfo>::finished, this,
                [this, currentRequestId, watcher,
                 userParameters = std::move(userParameters)]() {
                    const auto loadedTask = watcher->future().result();
                    if (currentRequestId >= m_latestRequestId.load()) {
                        emit latestLoadingFinished(
                            QVariant::fromValue(std::move(userParameters)),
                            currentRequestId, loadedTask);
                    }
                    emit anyLoadingFinished(currentRequestId, loadedTask);

                    // Clean-up, remove stored watcher.
                    m_activeWatchers.remove(currentRequestId);
                    watcher->deleteLater();
                });

        // Launching thread.
        static const auto fetchTaskInfo =
            [](const QString &pathToBinary,
               const DetailedTaskInfo &sourceTask) {
                try {
                    // Note, use ONLY reading outside class Taskwarrior
                    // otherwise UNDO will be broken.
                    const TaskWarriorExecutor executor(pathToBinary);
                    DetailedTaskInfo task(sourceTask.task_id);
                    task.execReadExisting(executor);
                    return task;
                } catch (...) { // NOLINT
                }
                return sourceTask;
            };
        const QFuture<DetailedTaskInfo> future = QtConcurrent::run(
            fetchTaskInfo, ConfigManager::config().getTaskBin(), partialTask);
        watcher->setFuture(future);

        return currentRequestId;
    }

  signals:
    /// @brief Signal sent when we thing we finished latest requested loading if
    /// it was many.
    void latestLoadingFinished(QVariant userParams, RequestId requestId,
                               DetailedTaskInfo result);
    /// @brief Signal is sent for any loading finished (including latest one).
    void anyLoadingFinished(RequestId requestId, DetailedTaskInfo result);

  private:
    using TaskFutWatcher = QFutureWatcher<DetailedTaskInfo>;
    explicit AsyncTaskLoader(QObject *parent = nullptr)
        : QObject{ parent }
    {
    }

    std::atomic<RequestId> m_latestRequestId{ 0u };
    QMap<RequestId, TaskFutWatcher *> m_activeWatchers;
};
