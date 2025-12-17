#ifndef QTUTIL_HPP
#define QTUTIL_HPP

#include <QAction>
#include <QDate>
#include <QDateTime>
#include <QString>
#include <QTextDocument>

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

/**
 * * @returns true if Ok-Cancel order of the buttons should be used (Ok/Positive
 * is 1st).
 */
bool isOkCancelOrder();

/// @returns true if @p what has string convertable to type int with decimal
/// base.
bool isInteger(const QString &what);

/// @brief sets @p whole_text as content of @p document.
/// @param whole_text is treated as markdown if supported by current Qt
/// installed or plain text otherwise.
/// @param document object to set text to.
void setContentOfTextDocument(QTextDocument &document,
                              const QString &whole_text);
#endif // QTUTIL_HPP
