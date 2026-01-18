#pragma once

#include "task.hpp"
#include "task_date_time.hpp"

#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QString>
#include <QStringLiteral>

#include <cstdint>
#include <utility>

class StatusEmoji {
  public:
    /// @brief Urgency is used as overall indicator, it should be checked for
    /// single top task in the list. It can be used for system tray icon.
    enum class EmojiUrgency : std::uint8_t {
        None = 0,
        Future,
        WaitPast,
        DueApproaching,   // We assume Due is something strategical (long time).
        Active,           // 🔵
        SchedApproaching, // We assume Sched is something "tactical" (short
                          // time)
        SchedPast,        // 🚀
        Overdue           // 🔥
    };

    explicit StatusEmoji(DetailedTaskInfo task,
                         QDateTime now = QDateTime::currentDateTime())
        : task(std::move(task))
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

    [[nodiscard]] EmojiUrgency getMostUrgentLevel() const
    {
        // Highest priority - missing deadline.
        if (task.due.get().has_value() &&
            task.due.get().relationToNow(now) == DatesRelation::Past) {
            return EmojiUrgency::Overdue;
        }

        // Something was scheduled and it is in the past now.
        if (task.sched.get().has_value() &&
            task.sched.get().relationToNow(now) == DatesRelation::Past) {
            return EmojiUrgency::SchedPast;
        }

        // Sched approaching
        if (task.sched.get().has_value() &&
            task.sched.get().relationToNow(now) == DatesRelation::Approaching) {
            return EmojiUrgency::SchedApproaching;
        }

        // Working on something...
        if (isSpecialSched()) {
            return EmojiUrgency::Active;
        }

        // Coming soon deadline.
        if (task.due.get().has_value() &&
            task.due.get().relationToNow(now) == DatesRelation::Approaching) {
            return EmojiUrgency::DueApproaching;
        }

        // Missing WAIT.
        if (task.wait.get().has_value() &&
            task.wait.get().relationToNow(now) == DatesRelation::Past) {
            return EmojiUrgency::WaitPast;
        }

        // Something set to future...
        if (task.due.get().has_value() || task.sched.get().has_value()) {
            return EmojiUrgency::Future;
        }

        return EmojiUrgency::None;
    }

    static QString urgencyToEmoji(const EmojiUrgency level)
    {
        switch (level) {
        case EmojiUrgency::Overdue:
            return QString::fromUtf8("🔥");
        case EmojiUrgency::Active:
            return QString::fromUtf8("🔵");
        case EmojiUrgency::SchedPast:
            return QString::fromUtf8("🚀");
        case EmojiUrgency::SchedApproaching:
            return QString::fromUtf8("⏳");
        case EmojiUrgency::DueApproaching:
            return QString::fromUtf8("⚠️");
        case EmojiUrgency::WaitPast:
            return QString::fromUtf8("✨");
        case EmojiUrgency::Future:
            return QString::fromUtf8("💤");
        case EmojiUrgency::None:
            [[fallthrough]];
        default:
            return {};
        }
    }
    static bool hasEmoji()
    {
        static const bool has_emoji = []() {
            const QFont font = QGuiApplication::font();
            const QFontMetrics metrics(font);
            return metrics.inFontUcs4(0x1F525); // 🔥
        }();
        return has_emoji;
    }

  private:
    DetailedTaskInfo task;
    QDateTime now;

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
                return hasEmoji() ? QString::fromUtf8("✨")
                                  : QStringLiteral(" * ");
            }
            return hasEmoji() ? QString::fromUtf8("⏸️") : QStringLiteral(" z ");
        }

        return {};
    }
};
