#include "pereodic_async_executor.hpp"
#include "qt_base_test.hpp"

#include <atomic>
#include <chrono>
#include <tuple>

#include <QEventLoop>
#include <QThread>

#include <gtest/gtest.h>

using namespace ::testing;
using namespace std::chrono_literals;

class PereodicAsynExecTest : public QtBaseTest {};

TEST_F(PereodicAsynExecTest, SimpleExecutionFlow)
{
    QEventLoop loop;
    int callCount = 0;
    auto callable = [](int a, int b) { return a + b; };
    auto provider = []() { return std::make_tuple(10, 20); };
    auto receiver = [&](int result) {
        EXPECT_EQ(result, 30);
        callCount++;
        loop.quit();
    };

    PereodicAsynExec exec(1000ms, callable, provider, receiver);
    exec.execNow();
    loop.exec();
    EXPECT_EQ(callCount, 1);
    loop.exec();
    EXPECT_EQ(callCount, 2);
}

TEST_F(PereodicAsynExecTest, PeriodicRestart)
{
    SCOPED_TRACE("Check that timer runs thread pereodically.");
    QEventLoop loop;
    int callCount = 0;

    auto callable = []() { return 0; };
    auto provider = []() { return std::make_tuple(); };

    auto receiver = [&](int) {
        callCount++;
        if (callCount == 3) {
            loop.quit();
        }
    };

    PereodicAsynExec exec(10ms, callable, provider, receiver);
    exec.execNow();
    loop.exec();

    EXPECT_EQ(callCount, 3);
}

TEST_F(PereodicAsynExecTest, AvoidsOverlapping)
{
    SCOPED_TRACE(
        "Check that fast calls to execNow() are ignored if thread is running.");
    QEventLoop loop;
    std::atomic<int> startCount{ 0 };

    // Long callable
    auto callable = [&](int) {
        startCount++;
        QThread::msleep(400);
        return 0;
    };
    auto provider = []() { return std::make_tuple(1); };
    auto receiver = [&](int) { loop.quit(); };

    PereodicAsynExec exec(10ms, callable, provider, receiver);

    exec.execNow();
    QThread::msleep(100);
    exec.execNow();
    loop.exec();
    QThread::msleep(100);
    EXPECT_EQ(startCount.load(), 1);
}

TEST_F(PereodicAsynExecTest, DestructorSafety)
{
    std::atomic<bool> threadStarted{ false };
    std::atomic<bool> threadFinished{ false };

    auto callable = [&](int) {
        threadStarted = true;
        QThread::msleep(300);
        threadFinished = true;
        return 42;
    };
    auto provider = []() { return std::make_tuple(1); };
    auto receiver = [](int) { /* Nothing*/ };
    {
        PereodicAsynExec exec(100ms, callable, provider, receiver);
        exec.execNow();

        // Wait while thread is started.
        while (!threadStarted) {
            QCoreApplication::processEvents();
        }
        SCOPED_TRACE(
            "Destructor is out of scope and must block until thread finished.");
    }
    EXPECT_TRUE(threadFinished) << "Callable had to finish.";
}
