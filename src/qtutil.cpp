#include "qtutil.hpp"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QString>

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
