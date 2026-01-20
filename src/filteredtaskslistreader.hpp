#pragma once

#include "allatoncekeywordsfinder.hpp"
#include "tabular_stencil_base.hpp"
#include "task.hpp"

#include <QStringList>

/// @brief Commands to read tasks list. Tasks will have particulary filled data
/// fields, and they should be read in full later if details are needed.
class FilteredTasksListReader : protected TabularStencilBase<DetailedTaskInfo> {
  public:
    using Response = TabularStencilBase<DetailedTaskInfo>::Response;
    Response tasks;

  public:
    explicit FilteredTasksListReader(AllAtOnceKeywordsFinder filter);

    /// @brief Queries taskwarrior for list of the tasks. Result is sorted by
    /// internal urgency.
    [[nodiscard]]
    bool readUrgencySortedTaskList(const TaskWarriorExecutor &executor);

    // TabularStencilBase interface
  protected:
    [[nodiscard]]
    const ColumnsSchema &getSchema() const override;

    [[nodiscard]]
    QStringList // NOLINT
    createCmdParameters(const TableStencil &stencil) const override;

  private:
    AllAtOnceKeywordsFinder m_filter;
};
