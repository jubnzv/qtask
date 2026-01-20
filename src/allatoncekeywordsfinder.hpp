#pragma once

#include <QString>
#include <QStringList>

#include <optional>

#include "taskwarriorexecutor.hpp"

/// @brief Keywords set by user. `task` finds all IDs which contain ALL
/// keywords.
class AllAtOnceKeywordsFinder {
  public:
    explicit AllAtOnceKeywordsFinder(QStringList keywords); // NOLINT

    [[nodiscard]]
    bool readIds(const TaskWarriorExecutor &executor);

    /// @returns std::nullopt if it was no filter applied, empty string if it
    /// was no task found by keywords, or task ids list.
    [[nodiscard]]
    const auto &getIds() const
    {
        return m_ids;
    }

    /// @returns true if nothing was found, i.e. it should be displayed empty
    /// results.
    [[nodiscard]]
    bool isNotFound() const
    {
        return getIds().has_value() && getIds()->isEmpty();
    }

  private:
    QStringList m_user_keywords;
    std::optional<QString> m_ids;
};
