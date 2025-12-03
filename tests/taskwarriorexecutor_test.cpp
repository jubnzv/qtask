#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <QString>

#include <gtest/gtest.h>
#include <stdexcept>

namespace
{
// Defined by CMake.
const QString kTaskCommand = TASK_EXECUTABLE_PATH;

} // namespace

namespace Test
{
using TaskWarriorExecutorTest = ::testing::Test;

TEST_F(TaskWarriorExecutorTest, ThrowsIfNotFound)
{
    static const auto alloc = []() { return TaskWarriorExecutor("abrvalg!"); };
    EXPECT_THROW(alloc(), std::runtime_error);
}

TEST_F(TaskWarriorExecutorTest, WorksIfFound)
{
    const TaskWarriorExecutor executor(kTaskCommand);
    EXPECT_FALSE(executor.getTaskVersion().isEmpty());
}

TEST_F(TaskWarriorExecutorTest, TaskWarriorDbStateUpdates)
{
    const TaskWarriorExecutor executor(kTaskCommand);
    EXPECT_FALSE(executor.getTaskVersion().isEmpty());

    const TaskWarriorDbState def;
    const auto read = TaskWarriorDbState::readCurrent(executor);

    ASSERT_TRUE(read);
    EXPECT_TRUE(read->isDifferent(def)) // NOLINT
        << "Note, this test may fail IF you have fresh installed TaskWarrior "
           "in the system without tasks on it.";
}

} // namespace Test
