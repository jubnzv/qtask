#ifndef TASK_HPP
#define TASK_HPP

#include <QDate>
#include <QString>
#include <QStringList>
#include <QVariant>

struct AbstractTask {
    AbstractTask()
        : uuid("")
        , description("")
        , project("")
        , tags({})
        , sched(QVariant{})
        , due(QVariant{})
    {
    }

    QString uuid;
    QString description;
    QString project;
    QStringList tags;
    QVariant sched;
    QVariant due;
};

struct Task final : public AbstractTask {
    enum class Priority { Unset, L, M, H };
    static QString priorityToString(const Priority &p);
    static Priority priorityFromString(const QString &p);

    Task()
        : priority(Priority::Unset)
        , active(false)
        , wait(QVariant{})
    {
    }

    Priority priority;
    bool active;
    QVariant wait;

    /// Tags that will be removed at the next command.
    QStringList removed_tags;

    QStringList getCmdArgs() const;
};

Q_DECLARE_METATYPE(Task)

struct RecurringTask final : public AbstractTask {
    RecurringTask()
        : period("")
    {
    }

    // Recurring period with date suffix:
    // https://taskwarrior.org/docs/design/recurrence.html#special-month-handling
    QString period;
};

#endif // TASK_HPP
