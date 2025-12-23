#pragma once

#include <QDateTime>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <qnamespace.h>
#include <qtversionchecks.h>
#include <qtypes.h>

#include "task_date_time.hpp"

/// @brief Parser of date/time in "task" output.
/// This should be initialized with expected "lexems" (which
/// are space delimeted "words") count and indexes. If such lexems can be found
/// it will compose QDateTime out of it.
struct DateTimeParser {
    /// @brief Expected amount of lexems (words) in string. It is used for
    /// validation purpose.
    qsizetype expected_lexems_count;
    /// @brief Index of the "date" lexem (word).
    qsizetype date_lexem_index;
    /// @brief Index of the "time" lexem (word).
    qsizetype time_lexem_index;

    /// @returns QDateTime object if @p line_of_lexems matches this configured
    /// parameters and they produce valid ISODate, otherwise std::nullopt.

    template <ETaskDateTimeRole taRole>
    [[nodiscard]]
    TaskDateTime<taRole>
    parseDateTimeString(const QString &line_of_lexems) const
    {
        if (date_lexem_index >= expected_lexems_count ||
            time_lexem_index >= expected_lexems_count) {
            return TaskDateTime<taRole>{};
        }
        const auto lexems = splitSpaceSeparatedString(line_of_lexems);
        if (lexems.size() != expected_lexems_count) {
            return TaskDateTime<taRole>{};
        }
        return composeDateTime<taRole>(lexems.at(date_lexem_index),
                                       lexems.at(time_lexem_index));
    }

    /// @brief Splits space separated string into QStringList.
    /// @returns QStringList, each element is part of string between spaces or
    /// line ends.
    [[nodiscard]] QStringList static splitSpaceSeparatedString(
        const QString &args_as_space_separated_string)
    {
        static const QRegularExpression kWhitespaceRx("\\s+");
        return args_as_space_separated_string.split(kWhitespaceRx,
                                                    kSplitSkipEmptyParts);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    static constexpr auto kSplitSkipEmptyParts = Qt::SkipEmptyParts;
#else
    static constexpr auto kSplitSkipEmptyParts = QString::SkipEmptyParts;
#endif // QT_VERSION_CHECK
  private:
    /// @brief tries to compose 2 strings into QDateTime as ISODate.
    /// @returns QDateTime or std::nullopt if it couldn't make valid date.

    template <ETaskDateTimeRole taRole>
    [[nodiscard]]
    static TaskDateTime<taRole> composeDateTime(const QString &date,
                                                const QString &time)
    {
        auto dt = QDateTime::fromString(QString{ "%1T%2" }.arg(date, time),
                                        Qt::ISODate);
        if (dt.isValid()) {
            return TaskDateTime<taRole>{ dt };
        }
        return TaskDateTime<taRole>{};
    }
};
