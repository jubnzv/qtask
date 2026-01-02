#pragma once

#include <atomic>
#include <tuple>
#include <type_traits>

#include <QFuture>
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QtConcurrent> //NOLINT
#include <qnamespace.h>

class IPereodicExec { // NOLINT
  public:
    virtual ~IPereodicExec() = default;
    virtual void execNow() = 0;
};

/// @tparam taPereodicCallable callable which executed in dedicated thread and
/// returns some value.
/// @tparam taPereodicParamsProvider callable which provides tuple of parameters
/// to call taPereodicCallable. Tuple is inpacked before call. It is called in
/// same thread from where execNow() is called. On timer call it will be GUI
/// thread.
/// @tparam taResultReceiver callable which is called in GUI thread and
/// should accept value returned by @tparam taPereodicCallable.
///
/// @note execNow() must be called at least once after construction explicit to
/// start tracking.
/// @note All callables should be valid until `this` exists.
template <typename taPereodicCallable, typename taPereodicParamsProvider,
          typename taResultReceiver>
class PereodicAsynExec : public IPereodicExec {
  public:
    using ParamsTuple = std::invoke_result_t<taPereodicParamsProvider>;
    using ReturnType = std::decay_t<decltype(std::apply(
        std::declval<taPereodicCallable>(), std::declval<ParamsTuple>()))>;

    PereodicAsynExec(const int kCheckPeriodMs, taPereodicCallable callable,
                     taPereodicParamsProvider params_provider,
                     taResultReceiver receiver)
        : m_callable(std::move(callable))
        , m_params_provider(std::move(params_provider))
        , m_result_receiver(std::move(receiver))
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
         * A steady, periodic timer risks starting a new  private:
    taPereodicCallable m_callable;
    taPereodicParamsProvider m_params_provider; check before the
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
            [this]() {
                static_assert(
                    !std::is_void_v<ReturnType>,
                    "void result of taPereodicCallable is NOT supported.");
                static_assert(std::is_invocable_v<taResultReceiver, ReturnType>,
                              "taResultReceiver signature is incompatible with "
                              "taPereodicCallable return type! "
                              "It must accept a parameter of type ReturnType.");
                m_result_receiver(m_future_reader.future().result());
                m_timer.start();
            },
            Qt::QueuedConnection);
    }

    PereodicAsynExec() = delete;
    PereodicAsynExec(const PereodicAsynExec &) = delete;
    PereodicAsynExec &operator=(const PereodicAsynExec &) = delete;
    PereodicAsynExec(PereodicAsynExec &&) = delete;
    PereodicAsynExec &operator=(PereodicAsynExec &&) = delete;

    ~PereodicAsynExec() override
    {
        // Granting that all lambdas will always have valid [this].
        m_timer.stop();
        if (m_future_reader.isRunning()) {
            m_future_reader.cancel();
            m_future_reader.waitForFinished();
        }
    }

    /// @brief Tries to execute check regardless of the current timer state.
    /// @note It is safe to call from any thread. taPereodicParamsProvider()
    /// will be called in the same thread as caller.
    void execNow() override
    {
        if (!testAndFlip(m_avoid_overlapping_execs, false)) {
            return;
        }
        if (!m_future_reader.isFinished()) {
            return;
        }

        const auto future = QtConcurrent::run(
            [this](auto tuple) {
                return std::apply(m_callable, std::move(tuple));
            },
            m_params_provider());

        m_future_reader.setFuture(future);
        m_avoid_overlapping_execs = false;
    }

  private:
    taPereodicCallable m_callable;
    taPereodicParamsProvider m_params_provider;
    taResultReceiver m_result_receiver;

    QFutureWatcher<ReturnType> m_future_reader;
    QTimer m_timer;
    std::atomic<bool> m_avoid_overlapping_execs{ false };

    static inline bool testAndFlip(std::atomic<bool> &var, const bool expected)
    {
        bool exp{ expected };
        return var.compare_exchange_strong(exp, !expected);
    }
};

/// @brief Factory function for the PereodicAsynExec.
/// @returns std::unique_ptr<PereodicAsynExec<C, P, R>>
template <typename C, typename P, typename R>
auto createPereodicAsynExec(int ms, C c, P p, R r)
{
    return std::make_unique<PereodicAsynExec<C, P, R>>(
        ms, std::move(c), std::move(p), std::move(r));
}
