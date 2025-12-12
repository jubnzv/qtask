#include "taskwatcher.hpp"

#include "configmanager.hpp"
#include "exec_on_exit.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <atomic>
#include <optional>

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
    , last_check_at(std::chrono::system_clock::now())
{
    /*
     * This reflects a crucial architectural trade-off.
     * * 1. Single-Shot (Current approach):
     * The timer is explicitly restarted upon both the timer's timeout and the
     * async operation's completion (checkNow/finished slots). This guarantees
     * that a new check WILL NOT overlap with the previous one.
     * The trade-off is a floating delay: the interval between checks will vary
     * (kCheckPeriod + async_duration), sacrificing precise timing for
     * reliability.
     * * 2. Multi-Shot (Rejected approach):
     * A steady, periodic timer risks starting a new check before the previous
     * asynchronous database reading (which takes ~100ms with 9 tasks present)
     * has finished, leading to race conditions or unnecessary overlapping
     * requests, especially when dealing with large databases.
     */
    m_check_for_changes_timer.setSingleShot(true);
    m_check_for_changes_timer.setInterval(kCheckPeriod);

    connect(&m_check_for_changes_timer, &QTimer::timeout, this,
            &TaskWatcher::checkNow);

    connect(
        m_state_reader, &QFutureWatcher<TaskWarriorDbState::Optional>::finished,
        this,
        [this]() {
            // This is GUI thread.
            using namespace std::chrono_literals;

            const auto now = std::chrono::system_clock::now();
            if (now - last_check_at > 60min) {
                enforceUpdate();
            }

            auto opt = m_state_reader->future().result();
            if (opt && opt->isDifferent(m_latestDbState)) {
                m_latestDbState = *opt;
                last_check_at = now;
                emit dataOnDiskWereChanged();
            }
            m_check_for_changes_timer.start();
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
                return TaskWarriorDbState::readCurrent(TaskWarriorExecutor(
                    pathToBinary,
                    TaskWarriorExecutor::TSkipBinaryValidation{}));
            } catch (...) { // NOLINT
            }
            return std::nullopt;
        },
        ConfigManager::config().getTaskBin());
    m_state_reader->setFuture(future);
}

void TaskWatcher::enforceUpdate()
{
    m_latestDbState = TaskWarriorDbState::invalidState();
}
