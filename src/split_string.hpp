#pragma once

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>

#include <initializer_list>

/// @brief Takes 1st word as key and everything else - as value.
/// Key must be aligned to begin of the string, any amount of space/tabs can
/// separated key/value.
struct SplitString {
    QString key;
    QString value;

    /// @param constructs initialized key/value pair out of given @p line.
    /// @note if string begins with spaces it will be invalid object (i.e. key
    /// will not be defined).
    explicit SplitString(const QString &line)
    {
        // (.*) ^ - string's begin
        // (\S+) - Group 1: non-space symbols (key / left word)
        // \s* - 0 or more space characters - delimeter
        // (.*) - Group 2 - all other symbols till the string end
        static const QRegularExpression kSplitRx("^(\\S+)\\s*(.*)");

        if (!line.isEmpty()) {
            const QRegularExpressionMatch match = kSplitRx.match(line);
            if (match.hasMatch()) {
                key = match.captured(1).simplified();
                value = match.captured(2).simplified();
            }
        }
    }

    SplitString(const std::initializer_list<QString> &) = delete;
    SplitString() = delete;

    /// @returns true if key was found / defined.
    [[nodiscard]]
    bool isValid() const
    {
        return !key.isEmpty();
    }
};
