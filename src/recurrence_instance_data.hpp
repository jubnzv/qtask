#pragma once

#include "lambda_visitors.hpp"

#include <QString>

#include <cassert>
#include <optional>
#include <variant>

/// @brief This file contains data related to reccurent instances (not
/// templates).

class RecurrentInstancePeriod {
  public:
    RecurrentInstancePeriod()
        : m_period{ Unknown{} }
    {
    }

    [[nodiscard]]
    bool operator==(const RecurrentInstancePeriod &other) const
    {
        return m_period == other.m_period;
    }

    [[nodiscard]]
    bool operator!=(const RecurrentInstancePeriod &other) const
    {
        return !(*this == other);
    }

    /// @returns true if object was initialized by parsing taskwarriror
    /// response.
    [[nodiscard]] bool isRead() const
    {
        static const LambdaVisitor visitor{
            [](const Unknown &) { return false; },
            [](const auto &) { return true; },
        };
        return std::visit(visitor, m_period);
    }

    /// @returns true if object has reccurency indicated on reading,
    /// not-read-yet objects will @return false.
    [[nodiscard]] bool isRecurrent() const
    {
        static const LambdaVisitor visitor{
            [](const QString &) { return true; },
            [](const RecurrentButMissingPeriod &) { return true; },
            [](const NotRecurrent &) { return false; },
            [](const Unknown &) { return false; },
        };
        return std::visit(visitor, m_period);
    }

    /// @returns true if object is fully read from taskwarrior.
    [[nodiscard]] bool isFullyRead() const
    {
        static const LambdaVisitor visitor{
            [](const QString &) { return true; },
            [](const NotRecurrent &) { return true; },

            [](const RecurrentButMissingPeriod &) { return false; },
            [](const Unknown &) { return false; },
        };
        return std::visit(visitor, m_period);
    }

    /// @brief Set object as non-reccurent, this means object does not require
    /// more reading.
    void setNonRecurrent() { m_period = NotRecurrent{}; }

    /// @brief Set object as reccurent, however we still need to read exact
    /// period later.
    void setRecurrent() { m_period = RecurrentButMissingPeriod{}; }

    /// @brief Set object as reccurent and set exact known @p period, this means
    /// object does not require more reading.
    void setRecurrent(const QString &period)
    {
        assert(!period.isEmpty());
        m_period = period;
    }

    /// @returns period string if it present or std::nullopt otherwise.
    [[nodiscard]] std::optional<QString> period() const
    {
        if (const auto *p = std::get_if<QString>(&m_period)) {
            return *p;
        }
        return std::nullopt;
    }

  private:
    struct Unknown {
        constexpr bool operator==(const Unknown &) const { return true; }
    };
    struct NotRecurrent {
        constexpr bool operator==(const NotRecurrent &) const { return true; }
    };
    // It may happen on particular read when taskwarrior just returns short
    // mark.
    struct RecurrentButMissingPeriod {
        constexpr bool operator==(const RecurrentButMissingPeriod &) const
        {
            return true;
        }
    };
    using RecurrencePeriod =
        std::variant<Unknown, NotRecurrent, RecurrentButMissingPeriod, QString>;

    RecurrencePeriod m_period;
};
