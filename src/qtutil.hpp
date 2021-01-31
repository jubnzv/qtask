#ifndef QTUTIL_HPP
#define QTUTIL_HPP

#include <QAction>
#include <QDate>
#include <QDateTime>

/// Adds possible shortcut information to the tooltip of the action.
/// This provides consistent behavior both with default and custom tooltips
/// when used in combination with removeShortcutToToolTip()
void addShortcutToToolTip(QAction *action);

/// Removes possible shortcut information from the tooltip of the action.
/// This provides consistent behavior both with default and custom tooltips
/// when used in combination with addShortcutToToolTip()
void removeShortcutFromToolTip(QAction *action);

/// QDateTime::startOfDay for the deprecated Qt.
/// See: https://doc.qt.io/qt-5/qdate.html#startOfDay
QDateTime startOfDay(const QDate &);

#endif // QTUTIL_HPP
