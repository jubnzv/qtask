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

} // namespace Test
