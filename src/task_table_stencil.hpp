#pragma once

#include <QString>
#include <QStringList>
#include <qtypes.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

/// @brief This class prepares columns and dividers request part,
/// and splits line in response per columns when communicate with taskwarrior
/// over console input/output.
class TableStencil {
  public:
    explicit TableStencil(QStringList columns_headers) // NOLINT
        : columns_headers(std::move(columns_headers))
    {
        if (this->columns_headers.empty()) {
            throw std::runtime_error("Empty columns list is not supported.");
        }
    }

    /// @returns columns list usable to pass to taskwarrior cmd.
    /// @note It is query preparation step.
    [[nodiscard]]
    QString getCmdColumns() const
    {
        return columns_headers.join(",");
    }

    /// @returns labels (marks) of the columns to be used, those are printed by
    /// taskwarrior once and allow us to find out column borders.
    /// @note It is query preparation step.
    [[nodiscard]]
    QString getCmdLabels() const
    {
        // First column will not have label, so it is easier to parse.
        const QStringList markers(columns_headers.size() - 1, kColumnMarker);
        return QString(",%1").arg(markers.join(","));
    }

    /// @brief find outs actual borders of the columns in response. It must be
    /// called before anything else when parsing response.
    /// @returns true if columns amount matches to expected.
    /// @note It is response parsing step.
    [[nodiscard]]
    bool processLabelString(const QString &labelString)
    {
        TColumnBorder currentBorder{ 0, 0 };
        columns_borders.clear();
        columns_borders.reserve(columns_headers.size());

        const auto findNextPos =
            [&labelString](const TColumnBorder &border,
                           const qsizetype skipMarkerItself) {
                return labelString.indexOf(kColumnMarker,
                                           border.starts_at + skipMarkerItself);
            };

        static const auto labelLen = kColumnMarker.size();
        for (qsizetype pos = findNextPos(currentBorder, 0); pos != -1;
             pos = findNextPos(currentBorder, labelLen)) {
            currentBorder.updateByMarkerPos(pos);
            columns_borders.emplace_back(currentBorder);
            currentBorder.next();
        }
        // Last column does not have label after it.
        currentBorder.length = std::numeric_limits<qsizetype>::max();
        columns_borders.emplace_back(currentBorder);
        return columns_borders.size() == columns_headers.size();
    }

    /// @brief Iterates columns in @p line and calls @p callback for each
    /// column. Columns were created by call to processLabelString() before.
    /// @tparam taCallable - Callable which should accept column index, column
    /// name, column value.
    /// @throws if parsed borders do no match required headers.
    template <typename taCallable>
    void forEachColumn(const QString &line, const taCallable &callback) const
    {
        const std::size_t count = columns_borders.size();
        if (count != static_cast<std::size_t>(columns_headers.size()) ||
            count == 0) {
            throw std::runtime_error(
                "processLabelString() was not called before or failed.");
        }

        const qsizetype lineLen = line.length();

        for (std::size_t i = 0; i < count; ++i) {
            const auto &border = columns_borders.at(i);
            QString value;
            if (border.starts_at < lineLen) {
                const qsizetype actualLength =
                    std::min(border.length, lineLen - border.starts_at);
                value = line.mid(border.starts_at, actualLength).trimmed();
            } else {
                value = "";
            }
            callback(static_cast<qsizetype>(i),
                     columns_headers.at(static_cast<qsizetype>(i)), value);
        }
    }

  private:
    struct TColumnBorder {
        qsizetype starts_at;
        qsizetype length;

        void updateByMarkerPos(qsizetype pos)
        {
            assert(pos != -1);
            length = pos - starts_at;
        }

        void next()
        {
            starts_at += length;
            length = 0;
        }
    };

    QStringList columns_headers; // NOLINT
    std::vector<TColumnBorder> columns_borders;

    inline static const QString kColumnMarker = "|";
};
