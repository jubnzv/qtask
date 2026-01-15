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

class StatusEmoji {
  public:
    explicit StatusEmoji(const DetailedTaskInfo &task,
                         QDateTime now = QDateTime::currentDateTime())
        : task(task)
        , now(std::move(now))
    {
    }

    [[nodiscard]]
    QString dueEmoji() const
    {
        if (task.due.get().has_value()) {
            return relationToEmoji(task.due.get(), now);
        }
        return {};
    }

    [[nodiscard]]
    QString waitEmoji() const
    {
        if (task.wait.get().has_value()) {
            return relationToEmoji(task.wait.get(), now);
        }
        return {};
    }

    [[nodiscard]]
    QString schedEmoji() const
    {
        if (isSpecialSched()) {
            return hasEmoji() ? QString::fromUtf8("🔵") : "[W]";
        }
        if (task.sched.get().has_value()) {
            return relationToEmoji(task.sched.get(), now);
        }
        return {};
    }

    [[nodiscard]]
    bool isSpecialSched() const
    {
        return task.active.get();
    }

  private:
    const DetailedTaskInfo &task;
    QDateTime now;

    static bool hasEmoji()
    {
        static const bool has_emoji = []() {
            const QFont font = QGuiApplication::font();
            const QFontMetrics metrics(font);
            return metrics.inFontUcs4(0x1F525); // 🔥
        }();
        return has_emoji;
    }

    /// @brief Computes DatesRelation between @p taskDate and @p now and
    /// converts to emoji if supported, to text string otherwise.
    /// @returns string oe emojies or text.
    template <ETaskDateTimeRole taRole>
    [[nodiscard]] QString static relationToEmoji(
        const TaskDateTime<taRole> &taskDate,
        const QDateTime &now = QDateTime::currentDateTime())
    {
        const auto rel = taskDate.relationToNow(now);

        if constexpr (taRole == ETaskDateTimeRole::Due) {
            switch (rel) {
            case DatesRelation::Past:
                return hasEmoji() ? QString::fromUtf8("🔥")
                                  : QStringLiteral("[!]");
            case DatesRelation::Approaching:
                return hasEmoji() ? QString::fromUtf8("⚠️")
                                  : QStringLiteral("(>)");
            case DatesRelation::Future:
                return hasEmoji() ? QString::fromUtf8("🗓️")
                                  : QStringLiteral("[ ]");
            }
        } else if constexpr (taRole == ETaskDateTimeRole::Sched) {
            switch (rel) {
            case DatesRelation::Past:
                return hasEmoji() ? QString::fromUtf8("🚀")
                                  : QStringLiteral("(+)");
            case DatesRelation::Approaching:
                return hasEmoji() ? QString::fromUtf8("⏳")
                                  : QStringLiteral("(:)");
            case DatesRelation::Future:
                return hasEmoji() ? QString::fromUtf8("💤")
                                  : QStringLiteral("...");
            }
        } else if constexpr (taRole == ETaskDateTimeRole::Wait) {
            if (rel == DatesRelation::Past) {
                return hasEmoji() ? QString::fromUtf8("👁️")
                                  : QStringLiteral(" * ");
            }
            return hasEmoji() ? QString::fromUtf8("⏸️") : QStringLiteral(" z ");
        }

        return {};
    }
};
