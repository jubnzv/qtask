#include "undo_tracker.hpp"
#include "exec_on_exit.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <cstdint>
#include <memory>
#include <utility>

#include <QObject>
#include <qtmetamacros.h>

UndoTracker::UndoTracker(std::shared_ptr<TaskWarriorExecutor> executor,
                         QObject *parent)
    : QObject{ parent }
    , m_counter{ 0u }
    , m_base_value{ kInvalidBaseState }
    , m_latest_db_reading{ kInvalidBaseState - 1 }
    , m_executor(std::move(executor))
{
}

void UndoTracker::startTracking()
{
    const exec_on_exit sendSignal([this]() { sendSignalForUi(); });
    if (m_executor) {
        const auto optState = TaskWarriorDbState::readCurrent(*m_executor);
        if (optState) {
            setVariablesByReading(optState->getUndoCount());
            return;
        }
    }
    setVariablesByReading(kInvalidBaseState);
}

void UndoTracker::newDbReading(const std::uint64_t undoCount)
{
    m_latest_db_reading = undoCount;
    if (!isSyncedCounter()) {
        setVariablesByReading(undoCount);
        sendSignalForUi();
    }
}

void UndoTracker::addUndo()
{
    // We need to increment m_latest_db_reading so sendSignalForUi() will not
    // fail right now.
    ++m_counter;
    ++m_latest_db_reading;
    sendSignalForUi();
}

bool UndoTracker::undo()
{
    if (!m_executor || m_counter == 0) {
        return false;
    }

    const exec_on_exit sendSignal([this]() { sendSignalForUi(); });
    if (isSyncedCounter() &&
        m_executor->execTaskProgramWithDefaults({ "undo" })) {
        --m_counter;
        // Decrement, so sendSignalForUi() will not fail right now.
        --m_latest_db_reading;
        return true;
    }
    return false;
}

void UndoTracker::sendSignalForUi()
{
    emit undoAvail(isSyncedCounter() && m_counter > 0);
}

void UndoTracker::setVariablesByReading(std::uint64_t reading)
{
    m_counter = 0;
    m_base_value = m_latest_db_reading = reading;
}

bool UndoTracker::isSyncedCounter() const
{
    // The logic is simple: what we see in the DB (latest_db_reading) must
    // exactly match what we expect (base_value + our_local_changes).
    // If they differ, an external process has modified the Taskwarrior
    // database.
    return m_base_value != kInvalidBaseState &&
           m_latest_db_reading == m_base_value + m_counter;
}
