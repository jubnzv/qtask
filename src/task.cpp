#include "task.hpp"

#include <QDateTime>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace
{
QString escapeTag(QString cleanedTag)
{
    cleanedTag.replace(QRegularExpression(R"(\W)"), QString("_"));
    cleanedTag.replace(QRegularExpression(R"(_+)"), QString("_"));
    if (!cleanedTag.isEmpty() && cleanedTag.at(0).digitValue() != -1) {
        // Tag cannot start with digit or everything is going bad...
        cleanedTag = "T_" + cleanedTag;
    }
    return cleanedTag;
}
} // namespace

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
    if (p == "L") {
        return Priority::L;
    }
    if (p == "M") {
        return Priority::M;
    }
    if (p == "H") {
        return Priority::H;
    }
    return Priority::Unset;
}

QStringList Task::getCmdArgs() const
{
    QStringList result;

    if (priority != Priority::Unset) {
        result << QString{ "pri:'%1'" }.arg(Task::priorityToString(priority));
    } else {
        result << QString{ "pri:''" };
    }

    result << QString{ "project:'%1'" }.arg(project);
    for (auto const &t : tags) {
        if (!t.isEmpty()) {
            const QString t_escpaed = escapeTag(t);
            if (!t_escpaed.isEmpty()) {
                result << QString{ "+%1" }.arg(t_escpaed);
            }
        }
    }
    for (auto const &t : removed_tags) {
        if (!t.isEmpty()) {
            const QString t_escpaed = escapeTag(t);
            if (!t_escpaed.isEmpty()) {
                result << QString{ "-%1" }.arg(t_escpaed);
            }
        }
    }

    static const QDateTime null_dt;
    result << QString{ "sched:%1" }.arg(
        sched.value_or(null_dt).toString(Qt::ISODate));
    result << QString{ "due:%1" }.arg(
        due.value_or(null_dt).toString(Qt::ISODate));
    result << QString{ "wait:%1" }.arg(
        wait.value_or(null_dt).toString(Qt::ISODate));

    QString escaped_desc = description;
    escaped_desc.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped_desc.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    result << QString{ "\"%1\"" }.arg(escaped_desc);

    return result;
}

void AbstractTask::unescapeStoredDescription()
{
    if (!description.isEmpty()) {
        description.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
        description.replace(QStringLiteral("\\\""), QStringLiteral("\""));
    }
}
