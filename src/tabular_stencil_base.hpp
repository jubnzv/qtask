#pragma once

#include "task_table_stencil.hpp"
#include "taskwarriorexecutor.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <variant>
#include <vector>

#include <QList>
#include <QString>
#include <QStringList>
#include <qtypes.h>

/// @brief Base class to request and parse tables in output.
/// It must be subclassed, children are responsible to provide details.
template <typename taDataClass>
class TabularStencilBase {
  public:
    using data_type = taDataClass;
    using Response = QList<taDataClass>;
    struct ErrorParsingLabels {};
    using ResponseOrError =
        std::variant<Response, TaskWarriorExecutor::TExecError,
                     ErrorParsingLabels>;

    /// @brief Calls `task` binary with parameters provided by children, parses
    /// it, accounting multi-line output and parses it to the list of @tparam
    /// taDataClass.
    /// @returns List<taDataClass> or errors.
    /// @note It is important that 1st column in list (index 0) will not get
    /// "continuation", i.e. it must not be printed over couple rows. "ID" is
    /// good candidate to be 1st.
    ResponseOrError readAndParseTable(const TaskWarriorExecutor &executor) const
    {
        const auto &schema = getSchema();
        const auto columnNames = getColumnNames(schema);
        if (columnNames.empty()) {
            throw std::logic_error(
                "Empty ColumnsSchema was provided by child class.");
        }

        TableStencil stencil(columnNames);
        const auto cmdParams = createCmdParameters(stencil)
                               << "rc.verbose:label"
                               << "rc.print.empty.columns=yes"
                               << "rc.dateformat=Y-M-DTH:N:S";

        const auto res = executor.execTaskProgramWithDefaults(cmdParams);
        if (!res) {
            return res.getError();
        }

        auto stdOut = res.getStdout();
        stdOut.removeAll("");
        if (!isTaskSentData(stdOut)) {
            return QList<taDataClass>{}; // Empty response.
        }

        if (!stencil.processLabelString(stdOut.at(kRowIndexOfDividers))) {
            return ErrorParsingLabels{}; // Could not parse labels properly.
        }

        return parseConsoleOutput(schema, stencil, stdOut);
    }

    /// @returns true if task outputed something except header and footer.
    static bool isTaskSentData(const QStringList &task_output) // NOLINT
    {
        // Empty lines are expected to be removed at all for this function to
        // work.
        static constexpr qsizetype kRowsAmountWhenEmptyResponse =
            kHeadersSize + kFooterSize;
        return task_output.size() > kRowsAmountWhenEmptyResponse;
    }

    // Expected values in reading TaskWarrior responses.
    static constexpr qsizetype kRowIndexOfDividers = 0;
    static constexpr qsizetype kHeadersSize = 2;
    static constexpr qsizetype kFooterSize = 1;

  protected:
    struct ColumnDescriptor {
        enum class LineMode : std::uint8_t {
            FirstLineOfNewRecord,
            AdditionalLineOfExistingRecord,
        };

        QString column_name;
        std::function<void(const QString /*value*/ &, taDataClass &,
                           LineMode lineMode)>
            column_handler;
    };
    using ColumnsSchema = std::vector<ColumnDescriptor>;

    /// @brief Children classes should return column's names (as it is sent to
    /// taskwarriror) with bound handlers.
    /// @note It is important that 1st column in list (index 0) will not get
    /// "continuation", i.e. it must not be printed over couple rows. "ID" is
    /// good candidate to be 1st.
    [[nodiscard]] virtual const ColumnsSchema &getSchema() const = 0;

    /// @brief Children must build and !@return final cmd parameters usable for
    /// taskwarrior call. @p stencil must be used to get lists of columns and
    /// labels for it.
    [[nodiscard]] virtual QStringList
    createCmdParameters(const TableStencil &stencil) const = 0;

    static QStringList getColumnNames(const ColumnsSchema &schema)
    {
        QStringList names;
        names.reserve(schema.size());
        std::transform(schema.begin(), schema.end(), std::back_inserter(names),
                       std::mem_fn(&ColumnDescriptor::column_name));
        return names;
    }

    static Response parseConsoleOutput(const ColumnsSchema &schema,
                                       const TableStencil &stencil,
                                       const QStringList &stdOut) // NOLINT
    {
        Response resp;
        resp.reserve(stdOut.size() - kHeadersSize - kFooterSize);

        // Pre-condition to detect continuation is that 1st column
        // is NEVER continues. It should be ensured by children.
        bool continuation = false;
        const auto addAndGet = [&resp]() -> taDataClass & {
            resp.emplace_back();
            return resp.last();
        };
        for (auto line_it = std::next(stdOut.begin(), kHeadersSize),
                  last_it = std::prev(stdOut.end(), kFooterSize);
             line_it != last_it; ++line_it) {
            const auto &line = *line_it;

            if (line.trimmed().isEmpty()) {
                continue;
            }
            stencil.forEachColumn(
                line, [&schema, &resp, &addAndGet, &continuation](
                          auto index, [[maybe_unused]] const auto &name,
                          const auto &value) {
                    continuation = index == 0 ? value.isEmpty() : continuation;
                    if (continuation && resp.empty()) {
                        // We got situation when output starts by
                        // continuation...impossible.
                        return;
                    }
                    const auto mode =
                        continuation
                            ? ColumnDescriptor::LineMode::
                                  AdditionalLineOfExistingRecord
                            : ColumnDescriptor::LineMode::FirstLineOfNewRecord;
                    auto &output =
                        continuation || index > 0 ? resp.last() : addAndGet();
                    schema.at(index).column_handler(value, output, mode);
                });
        }

        return resp;
    }
};
