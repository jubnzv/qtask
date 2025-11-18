#ifndef TASK_HPP
#define TASK_HPP

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <qdatetime.h>

#include <optional>

struct AbstractTask {
    using OptionalDateTime = std::optional<QDateTime>;

    QString uuid;
    QString description;
    QString project;
    QStringList tags;
    OptionalDateTime sched;
    OptionalDateTime due;

    /// @brief It MUST be called once description is read from "task".
    /// Because taskwarrior does escaping too.
    /// @note Placing it here because getCmdArgs() will do escaping. So they
    /// both are close and not forgotten.
    void unescapeStoredDescription();
};

struct Task final : public AbstractTask {
    enum class Priority { Unset, L, M, H };
    static QString priorityToString(const Priority &p);
    static Priority priorityFromString(const QString &p);

    Priority priority{ Priority::Unset };
    bool active{ false };
    OptionalDateTime wait;

    /// Tags that will be removed at the next command.
    QStringList removed_tags;

    QStringList getCmdArgs() const;
};

Q_DECLARE_METATYPE(Task)

struct RecurringTask final : public AbstractTask {
    // Recurring period with date suffix:
    // https://taskwarrior.org/docs/design/recurrence.html#special-month-handling
    QString period;
};

#endif // TASK_HPP
