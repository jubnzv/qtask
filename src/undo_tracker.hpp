#pragma once

#include "taskwarriorexecutor.hpp"

#include <cstdint>
#include <limits>
#include <memory>

#include <QObject>
#include <qtmetamacros.h>

/// @brief This class tracks undo counts. It uses value read from DB as base to
/// see, if it was any external action. If it was than we cannot make own undo.
class UndoTracker : public QObject {
    Q_OBJECT
  public:
    explicit UndoTracker(std::shared_ptr<TaskWarriorExecutor> executor,
                         QObject *parent = nullptr);

    /// @returns true if we're in sync to DB and can do undos.
    [[nodiscard]] bool isSyncedCounter() const;

    UndoTracker &operator++()
    {
        addUndo();
        return *this;
    }
  public slots:
    /// @brief Reads current state of data base and uses it as base line for
    /// accounting undos.
    /// @note It must be called at least once after creation. It does actual DB
    /// reading itself.
    void startTracking();

    /// @brief Other watchers should provide current undos count in DB (so we do
    /// not introduce extra reading here).
    /// Based on provided value we can decide if we can do own undo yet.
    void newDbReading(std::uint64_t undoCount);

    /// @brief It should be called when we did something, which increases undo
    /// count.
    void addUndo();

    /// @brief Executes UNDO operation DB IF we can.
    /// @returns true if undo was executed.
    bool undo();
  signals:
    /// @brief signal UI if it should enable/disable undo controls.
    void undoAvail(bool enabled);

  private:
    void sendSignalForUi();
    void setVariablesByReading(std::uint64_t reading);

    /// @brief The number of undoable actions performed by THIS session.
    /// It tells us how many times we can safely call 'undo'.
    std::uint64_t m_counter;
    /// @brief The DB's undo count at the moment tracking started or was reset.
    /// Used as a reference point to detect drift caused by external tools.
    std::uint64_t m_base_value;
    /// @brief The most recent known state of the DB's undo count.
    /// It is updated either by real DB readings or "optimistically" by our
    /// own addUndo/undo calls to prevent UI flicker during sync.
    std::uint64_t m_latest_db_reading;

    std::shared_ptr<TaskWarriorExecutor> m_executor;

    static constexpr auto kInvalidBaseState =
        std::numeric_limits<std::uint64_t>::max();
};
