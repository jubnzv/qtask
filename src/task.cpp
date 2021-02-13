#include "task.hpp"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVariant>

QString Task::priorityToString(const Priority &p)
{
    switch (p) {
    case Priority::L:
        return "L";
    case Priority::M:
        return "M";
    case Priority::H:
        return "H";
    default:
        return "";
    }
}

Task::Priority Task::priorityFromString(const QString &p)
{
    if (p == "L")
        return Priority::L;
    if (p == "M")
        return Priority::M;
    if (p == "H")
        return Priority::H;
    return Priority::Unset;
}

QStringList Task::getCmdArgs() const
{
    QStringList result;

    if (priority != Priority::Unset)
        result << QString{ " pri:'%1'" }.arg(Task::priorityToString(priority));
    else
        result << QString{ " pri:''" };

    result << QString{ " project:'%1'" }.arg(project);
    for (auto const &t : tags) {
        if (!t.isEmpty()) {
            QString t_escpaed = t;
            t_escpaed.replace("'", "");
            if (!t_escpaed.isEmpty())
                result << QString{ " +%1" }.arg(t_escpaed);
        }
    }
    for (auto const &t : removed_tags) {
        if (!t.isEmpty()) {
            QString t_escpaed = t;
            t_escpaed.replace("'", "");
            if (!t_escpaed.isEmpty())
                result << QString{ " -%1" }.arg(t_escpaed);
        }
    }

    result << QString{ " sched:%1" }.arg(
        sched.toDateTime().toString(Qt::ISODate));
    result << QString{ " due:%1" }.arg(due.toDateTime().toString(Qt::ISODate));
    result << QString{ " wait:%1" }.arg(
        wait.toDateTime().toString(Qt::ISODate));

    QString escaped_desc = description;
    escaped_desc.replace("'", "\'");
    result << QString{ " '%1'" }.arg(description);

    return result;
}
