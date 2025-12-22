#pragma once

#include <QDateTime>
#include <QString>
#include <qnamespace.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

/// @brief The role of the date-time value.
enum class ETaskDateTimeRole : std::uint8_t { Due, Sched, Wait };

/// @brief Defines how one date compares to other.
enum class DatesRelation : std::uint8_t {
    Past,        // Date passed.
    Approaching, // Date is soon happening, warnings can be showed.
    Future,      // Date is "normal", there is no special warnings.
};

/// @brief This template represents optional date time with assigned task's
/// role.
template <ETaskDateTimeRole taRole>
class TaskDateTime {
  public:
    TaskDateTime()
        : m_value(std::nullopt)
    {
    }
    explicit TaskDateTime(const QDateTime &dateTime)
        : m_value(dateTime)
    {
    }
    TaskDateTime(const TaskDateTime &other) = default;
    TaskDateTime(TaskDateTime &&other) = default;
    TaskDateTime &operator=(const TaskDateTime &other) = default;
    TaskDateTime &operator=(TaskDateTime &&other) = default;
    ~TaskDateTime() = default;

    TaskDateTime &operator=(const QDateTime &dt) noexcept
    {
        m_value = dt;
        return *this;
    }

    /// @returns true if date-time was set.
    [[nodiscard]]
    bool has_value() const noexcept
    {
        return m_value.has_value();
    }

    /// @returns ref to date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    QDateTime &operator*()
    {
        return value();
    }

    /// @returns const-ref to date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    const QDateTime &operator*() const
    {
        return value();
    }

    /// @returns ref to date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    QDateTime &value()
    {
        return deref(*this);
    }

    /// @returns const-ref to date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    const QDateTime &value() const
    {
        return deref(*this);
    }

    /// @returns pointer to date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    QDateTime *operator->()
    {
        return ptr(*this);
    }

    /// @returns pointer to const date-time value or
    /// @throws if optional was not set to date-time value.
    [[nodiscard]]
    const QDateTime *operator->() const
    {
        return ptr(*this);
    }

    /// @returns relation to given time moment @p now which can be used to
    /// display warnings.
    [[nodiscard]]
    DatesRelation
    relationToNow(const QDateTime &now = QDateTime::currentDateTime()) const
    {
        if (!has_value()) {
            return DatesRelation::Future;
        }
        constexpr std::chrono::milliseconds approachingMs = warning_interval();
        const auto diffMs = now.msecsTo(value());
        if (value() < now) {
            return DatesRelation::Past;
        }
        if (diffMs <= approachingMs.count()) {
            return DatesRelation::Approaching;
        }
        return DatesRelation::Future;
    }

    /// @returns warning interval which should be used for this role ( (now() +
    /// warning_interval())).
    static constexpr std::chrono::milliseconds warning_interval()
    {
        // TODO: make it configurable.
        using namespace std::chrono_literals;
        if constexpr (taRole == ETaskDateTimeRole::Sched) {
            return 15min; // NOLINT
        } else {
            return 24h; // NOLINT
        }
    }

    /// @returns new time value interval which should be used for this role when
    /// creating new record (now() + new_interval()).
    static constexpr std::chrono::seconds new_interval()
    {
        // TODO: make it configurable.
        using namespace std::chrono_literals;
        if constexpr (taRole == ETaskDateTimeRole::Sched) {
            return 1h; // NOLINT
        } else if constexpr (taRole == ETaskDateTimeRole::Due) {
            return 24h * 7; // NOLINT
        } else if constexpr (taRole == ETaskDateTimeRole::Wait) {
            return 24h * 14; // NOLINT
        } else {
            return 24h * 21; // NOLINT
        }
    }

    /// @returns current template parameter as string for logging purposes.
    static constexpr std::string_view role_name()
    {
        switch (taRole) {
        case ETaskDateTimeRole::Due:
            return "Due";
        case ETaskDateTimeRole::Sched:
            return "Sched";
        case ETaskDateTimeRole::Wait:
            return "Wait";
        }
        std::abort();
    }

  private:
    using OptionalDateTime = std::optional<QDateTime>;
    OptionalDateTime m_value;

    template <typename taSelf>
    static decltype(auto) deref(taSelf &self)
    {
        if (!self.has_value()) {
            throw std::logic_error(std::string("TaskDateTime<") +
                                   std::string(role_name()) + "> is not set");
        }
        return *self.m_value;
    }

    template <typename Self>
    static auto ptr(Self &self)
    {
        return std::addressof(deref(self));
    }
};

namespace task_date_time_format
{
template <ETaskDateTimeRole taRole>
QString formatForCmd(const TaskDateTime<taRole> &dt)
{
    constexpr auto prefix = []() constexpr -> const char * {
        if constexpr (taRole == ETaskDateTimeRole::Due) {
            return "due:";
        } else if constexpr (taRole == ETaskDateTimeRole::Sched) {
            return "sched:";
        } else if constexpr (taRole == ETaskDateTimeRole::Wait) {
            return "wait:";
        } else {
            return "";
        }
    }();

    return QString(prefix) +
           (dt.has_value() ? dt->toString(Qt::ISODate) : QString());
}

} // namespace task_date_time_format
