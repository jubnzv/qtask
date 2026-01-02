#pragma once

#include <atomic>
#include <chrono>
#include <type_traits>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QtConcurrent>
#include <qnamespace.h>

/// @note execNow() must be called at least once after construction explicit to
/// start tracking.
template <typename taPereodicCallable>
class PereodicAsynExec {
  public:
    using ReturnType = std::decay_t<std::invoke_result_t<taPereodicCallable>>;

    template <typename taResultReceiver>
    PereodicAsynExec(const int kCheckPeriodMs, taPereodicCallable callable,
                     taResultReceiver receiver)
        : m_callable(callable)
    {
        /*
         * setSingleShot(true) reflects a crucial architectural trade-off.
         * * 1. Single-Shot (Current approach):
         * The timer is explicitly restarted upon both the timer's timeout and
         * the async operation's completion (checkNow/finished slots). This
         * guarantees that a new check WILL NOT overlap with the previous one.
         * The trade-off is a floating delay: the interval between checks will
         * vary (kCheckPeriod + async_duration), sacrificing precise timing for
         * reliability.
         * * 2. Multi-Shot (Rejected approach):
         * A steady, periodic timer risks starting a new check before the
         * previous asynchronous database reading (which takes ~100ms with 9
         * tasks present) has finished, leading to race conditions or
         * unnecessary overlapping requests, especially when dealing with large
         * databases.
         */
        m_timer.setSingleShot(true);
        m_timer.setInterval(kCheckPeriodMs);
        QObject::connect(&m_timer, &QTimer::timeout, &m_timer,
                         [this]() { execNow(); });

        QObject::connect(
            &m_future_reader, &QFutureWatcher<ReturnType>::finished,
            &m_future_reader,
            [this, receiver]() {
                // This is GUI thread.
                if constexpr (!std::is_void_v<ReturnType>) {
                    receiver(m_future_reader.future().result());
                } else {
                    m_future_reader.future().result();
                    receiver();
                }
                m_timer.start();
            },
            Qt::QueuedConnection);
    }

    /// @brief Tries to execute check regardless of the current timer state.
    /// @note It is safe to call from any thread.
    void execNow()
    {
        if (!testAndFlip(m_exec_once, false)) {
            return;
        }
        if (!m_future_reader.isFinished()) {
            return;
        }
        const auto future = QtConcurrent::run(m_callable);
        m_future_reader.setFuture(future);
        m_exec_once = false;
    }

  private:
    taPereodicCallable m_callable;
    QFutureWatcher<ReturnType> m_future_reader;
    QTimer m_timer;
    std::atomic<bool> m_exec_once{ false };

    static inline bool testAndFlip(std::atomic<bool> &var, const bool expected)
    {
        bool exp{ expected };
        return var.compare_exchange_strong(exp, !expected);
    }
};
