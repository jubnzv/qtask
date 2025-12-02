#include "taskwatcher.hpp"

#include "configmanager.hpp"
#include "exec_on_exit.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <atomic>
#include <utility>

#include <QFuture>
#include <QObject>
#include <QtConcurrent>

namespace
{
///  @brief How often at most we will check for new data.
// TODO: add config settings in UI instead.
constexpr int kCheckPeriod = 10000;

// tests atomic bool for expected, if it is - sets it to !expected and returns
// true
inline bool test_and_flip(std::atomic<bool> &var, const bool expected)
{
    bool exp{ expected };
    return var.compare_exchange_strong(exp, !expected);
}
} // namespace

TaskWatcher::TaskWatcher(QObject *parent)
    : QObject(parent)
    , m_state_reader(new QFutureWatcher<TaskWarriorDbState::Optional>(this))
{
    m_check_for_changes_timer.setSingleShot(true);
    m_check_for_changes_timer.setInterval(kCheckPeriod);

    connect(&m_check_for_changes_timer, &QTimer::timeout, this,
            &TaskWatcher::checkNow);

    connect(
        m_state_reader, &QFutureWatcher<TaskWarriorDbState::Optional>::finished,
        this,
        [this]() {
            // This is GUI thread.
            auto opt = m_state_reader->future().result();
            if (opt && opt->isDifferent(m_latestDbState)) {
                m_latestDbState = *opt;
                m_check_for_changes_timer.start();
                emit dataOnDiskWereChanged();
            }
        },
        Qt::QueuedConnection);
}

void TaskWatcher::checkNow()
{
    // This slot is in progress of something, drop incoming request.
    if (!test_and_flip(m_slot_once, false)) {
        return;
    }
    if (!m_state_reader->isFinished()) {
        return;
    }
    const exec_on_exit when_slot_ends([this]() {
        m_check_for_changes_timer.start();
        m_slot_once = false;
    });

    const auto future = QtConcurrent::run(
        [](const auto &pathToBinary) -> TaskWarriorDbState::Optional {
            try {
                // This is non-GUI thread.
                return TaskWarriorDbState::readCurrent(
                    TaskWarriorExecutor(pathToBinary));
            } catch (...) { // NOLINT
            }
            return std::nullopt;
        },
        ConfigManager::config().getTaskBin());
    m_state_reader->setFuture(future);
}
