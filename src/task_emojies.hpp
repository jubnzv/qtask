#pragma once

#include "task.hpp"
#include "task_date_time.hpp"

#include <QDateTime>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QString>

#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QString>
#include <QStringLiteral>

/// @brief Computes DatesRelation between @p taskDate and @p now and converts to
/// emoji if supported, to text string otherwise.
/// @returns string oe emojies or text.
template <ETaskDateTimeRole taRole>
QString relationToEmoji(const TaskDateTime<taRole> &taskDate,
                        const QDateTime &now = QDateTime::currentDateTime())
{
    static const bool hasEmoji = []() {
        const QFont font = QGuiApplication::font();
        const QFontMetrics metrics(font);
        return metrics.inFontUcs4(0x1F525); // 🔥
    }();
    const auto rel = taskDate.relationToNow(now);

    if constexpr (taRole == ETaskDateTimeRole::Due) {
        switch (rel) {
        case DatesRelation::Past:
            return hasEmoji ? QString::fromUtf8("🔥") : QStringLiteral("[!]");
        case DatesRelation::Approaching:
            return hasEmoji ? QString::fromUtf8("⚠️") : QStringLiteral("(>)");
        case DatesRelation::Future:
            return hasEmoji ? QString::fromUtf8("📌") : QStringLiteral("[ ]");
        }
    } else if constexpr (taRole == ETaskDateTimeRole::Sched) {
        switch (rel) {
        case DatesRelation::Past:
            return hasEmoji ? QString::fromUtf8("📢") : QStringLiteral("(+)");
        case DatesRelation::Approaching:
            return hasEmoji ? QString::fromUtf8("⏳") : QStringLiteral("(:)");
        case DatesRelation::Future:
            return hasEmoji ? QString::fromUtf8("💤") : QStringLiteral("...");
        }
    } else if constexpr (taRole == ETaskDateTimeRole::Wait) {
        if (rel == DatesRelation::Past) {
            return hasEmoji ? QString::fromUtf8("👁️") : QStringLiteral(" * ");
        }
        return hasEmoji ? QString::fromUtf8("⏸️") : QStringLiteral(" z ");
    }

    return {};
}

/// @brief Converts 3 dates of the @p task to emojies using relationToEmoji()
/// and @returns joined result of 3 conversions.
/// @note If date was not set it will be skip.
inline QString
taskToTimeEmojies(const DetailedTaskInfo &task,
                  const QDateTime &now = QDateTime::currentDateTime())
{
    QStringList icons;
    if (task.due.get().has_value()) {
        icons << relationToEmoji(task.due.get(), now);
    }
    if (task.sched.get().has_value()) {
        icons << relationToEmoji(task.sched.get(), now);
    }
    if (task.wait.get().has_value()) {

        icons << relationToEmoji(task.wait.get(), now);
    }
    icons.removeAll(QString());
    return icons.join(" ");
}
