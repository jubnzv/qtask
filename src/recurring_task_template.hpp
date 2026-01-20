#pragma once

#include <QList>
#include <QString>

#include <optional>

#include "taskwarriorexecutor.hpp"

struct RecurringTaskTemplate {
    // Recurring period with date suffix:
    // https://taskwarrior.org/docs/design/recurrence.html#special-month-handling
    QString uuid;
    QString period;
    QString project;
    QString description;

    [[nodiscard]]
    static std::optional<QList<RecurringTaskTemplate>>
    readAll(const TaskWarriorExecutor &executor);
};
// Q_DECLARE_METATYPE(RecurringTaskTemplate);
