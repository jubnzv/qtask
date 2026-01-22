#include "taskwatcher.hpp"

#include "configmanager.hpp"
#include "pereodic_async_executor.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <QObject>
#include <QString>

#include <cassert>
#include <optional>
#include <tuple>
#include <utility>

namespace
{
using namespace std::chrono_literals;

///  @brief How often at most we will check for new data.
// TODO: add config settings in UI instead.
constexpr int kCheckPeriod = 10000;

} // namespace

TaskWatcher::TaskWatcher(QObject *parent)
    : QObject(parent)
{
    // We need to report that we detected disk change with delay, if it was us,
    // then DB needs time to settle.
    delayedSignalSender.setSingleShot(true);
    delayedSignalSender.setInterval(750);
    connect(&delayedSignalSender, &QTimer::timeout, this,
            &TaskWatcher::dataOnDiskWereChanged);

    auto threadBody = [](QString pathToBinary) -> TaskWarriorDbState::Optional {
        try {
            // This is non-GUI thread.
            return TaskWarriorDbState::readCurrent(TaskWarriorExecutor(
                std::move(pathToBinary),
                TaskWarriorExecutor::TSkipBinaryValidation{}));
        } catch (...) { // NOLINT
        }
        return std::nullopt;
    };
    auto paramsForThread = []() {
        QString path = ConfigManager::config().get(ConfigManager::TaskBin);
        return std::make_tuple(std::move(path));
    };
    auto receiverFromThread = [this](TaskWarriorDbState::Optional opt) {
        // This is GUI thread.
        if (opt && opt->isDifferent(m_latestDbState)) {
            m_latestDbState = *opt;
            delayedSignalSender.start();
        }
    };

    m_pereodic_worker = createPereodicAsynExec(
        kCheckPeriod, std::move(threadBody), std::move(paramsForThread),
        std::move(receiverFromThread));
}

void TaskWatcher::checkNow()
{
    assert(m_pereodic_worker);
    m_pereodic_worker->execNow();
}
