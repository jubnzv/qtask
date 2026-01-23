#pragma once

#include <QDateTime>
#include <QString>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include "qtutil.hpp"

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

    [[nodiscard]]
    bool operator==(const TaskDateTime &other) const noexcept
    {
        if (!has_value() && !other.has_value()) {
            return true;
        }
        if (has_value() != other.has_value()) {
            return false;
        }
        return value() == other.value();
    }

    [[nodiscard]]
    bool operator!=(const TaskDateTime &other) const noexcept
    {
        return !(*this == other);
    }

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
        const auto diffMs = std::chrono::milliseconds(now.msecsTo(value()));
        if (value() < now) {
            return DatesRelation::Past;
        }
        if (diffMs <= approachingMs) {
            return DatesRelation::Approaching;
        }
        return DatesRelation::Future;
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

    /// @returns date-time usable to create new UI element, it is now + offset
    /// according to role.
    [[nodiscard]]
    static QDateTime suggest_default_date_time()
    {
        using namespace std::chrono_literals;

        // kWorkDayBegins represents the start of the workday for Due & Wait
        // tasks. Due & Wait use day precision offsets, while Sched uses minute
        // precision.
        // TODO: make it configurable too.
        constexpr std::chrono::seconds kWorkDayBegins = 9h; // NOLINT
        constexpr std::chrono::seconds kCongiguredOffset = new_interval();

        if constexpr (taRole == ETaskDateTimeRole::Sched) {
            return QDateTime::currentDateTime().addSecs(new_interval().count());
        } else { // Due, Wait
            const QDateTime base = startOfDay(QDate::currentDate());
            return base.addSecs(kCongiguredOffset.count() +
                                kWorkDayBegins.count());
        }
    }

    /// @returns cmd prefix for the role.
    static constexpr const char *role_name_cmd()
    {
        switch (taRole) {
        case ETaskDateTimeRole::Due:
            return "due";
        case ETaskDateTimeRole::Sched:
            return "sched";
        case ETaskDateTimeRole::Wait:
            return "wait";
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

  public:
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
};
