#include "qtutil.hpp"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDate>
#include <QDateTime>
#include <QPalette>
#include <QString>
#include <QTimeZone>

// Guesses a descriptive text from a text suited for a menu entry.
// This is equivalent to QActions internal qt_strippedText().
static QString strippedActionText(QString s)
{
    s.remove(QString::fromLatin1("..."));
    for (int i = 0; i < s.size(); ++i) {
        if (s.at(i) == QLatin1Char('&'))
            s.remove(i, 1);
    }
    return s.trimmed();
}

void addShortcutToToolTip(QAction *action)
{
    if (!action->shortcut().isEmpty()) {
        QString tooltip = action->property("tooltipBackup").toString();
        if (tooltip.isEmpty()) {
            tooltip = action->toolTip();
            if (tooltip != strippedActionText(action->text())) {
                action->setProperty(
                    "tooltipBackup",
                    action->toolTip()); // action uses a custom tooltip. Backup
                                        // so that we can restore it later.
            }
        }
        QColor shortcutTextColor =
            QApplication::palette().color(QPalette::ToolTipText);
        QString shortCutTextColorName;
        if (shortcutTextColor.value() == 0) {
            shortCutTextColorName =
                "gray"; // special handling for black because lighter() does not
                        // work there [QTBUG-9343].
        } else {
            int factor = (shortcutTextColor.value() < 128) ? 150 : 50;
            shortCutTextColorName = shortcutTextColor.lighter(factor).name();
        }
        action->setToolTip(
            QString("<p style='white-space:pre'>%1&nbsp;&nbsp;<code "
                    "style='color:%2; font-size:small'>%3</code></p>")
                .arg(tooltip, shortCutTextColorName,
                     action->shortcut().toString(QKeySequence::NativeText)));
    }
}

void removeShortcutFromToolTip(QAction *action)
{
    action->setToolTip(action->property("tooltipBackup").toString());
    action->setProperty("tooltipBackup", QVariant());
}

// clang-format off
// https://github.com/qt/qtbase/tree/ae2c30942086bd0387c6d5297c0cb85b505f29a0/src/corelib/time/qdatetime.cpp#L762
static QDateTime toEarliest(QDate day, const QDateTime &form)
{
    const Qt::TimeSpec spec = form.timeSpec();
    const int offset = (spec == Qt::OffsetFromUTC) ? form.offsetFromUtc() : 0;
#if QT_CONFIG(timezone)
    QTimeZone zone;
    if (spec == Qt::TimeZone)
        zone = form.timeZone();
#endif
    auto moment = [=](QTime time) {
        switch (spec) {
        case Qt::OffsetFromUTC:
            return QDateTime(day, time, spec, offset);
#if QT_CONFIG(timezone)
        case Qt::TimeZone:
            return QDateTime(day, time, zone);
#endif
        default:
            return QDateTime(day, time, spec);
        }
    };
    // Longest routine time-zone transition is 2 hours:
    QDateTime when = moment(QTime(2, 0));
    if (!when.isValid()) {
        // Noon should be safe ...
        when = moment(QTime(12, 0));
        if (!when.isValid()) {
            // ... unless it's a 24-hour jump (moving the date-line)
            when = moment(QTime(23, 59, 59, 999));
            if (!when.isValid())
                return QDateTime();
        }
    }
    int high = when.time().msecsSinceStartOfDay() / 60000;
    int low = 0;
    // Binary chop to the right minute
    while (high > low + 1) {
        int mid = (high + low) / 2;
        QDateTime probe = moment(QTime(mid / 60, mid % 60));
        if (probe.isValid() && probe.date() == day) {
            high = mid;
            when = probe;
        } else {
            low = mid;
        }
    }
    return when;
}
// clang-format on

QDateTime startOfDay(const QDate &date)
{
    QDateTime when(date, QTime(0, 0), Qt::LocalTime);
    if (!when.isValid())
        when = toEarliest(date, when);
    return when.isValid() ? when : QDateTime();
}
