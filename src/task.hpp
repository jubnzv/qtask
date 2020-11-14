#ifndef TASK_HPP
#define TASK_HPP

#include <QDate>
#include <QString>
#include <QStringList>
#include <QVariant>

struct Task final {
  public:
    enum class Priority { Unset, L, M, H };
    static QString priorityToString(const Priority &p);
    static Priority priorityFromString(const QString &p);

  public:
    Task()
        : uuid("")
        , description("")
        , priority(Priority::Unset)
        , project("")
        , tags({})
        , active(false)
        , sched(QVariant{})
        , due(QVariant{})
        , wait(QVariant{})
    {
    }

    QString uuid;
    QString description;
    Priority priority;
    QString project;
    QStringList tags;
    bool active;
    QVariant sched;
    QVariant due;
    QVariant wait;

    /// Tags that will be removed at the next command.
    QStringList removed_tags;

    QStringList getCmdArgs() const;
};

#endif // TASK_HPP
