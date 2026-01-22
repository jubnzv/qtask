#pragma once

#include "task.hpp"

#include <functional>

/// @brief Usable in cases when we need current task short ID.
inline static const auto kTaskIdShortGetter =
    std::mem_fn(&DetailedTaskInfo::task_id);

/// @brief Unique, never changing long UUID. Usable in cases, like restore
/// selection after full reaload when short IDs could be changed external.
inline static const auto kTaskUuidGetter =
    std::mem_fn(&DetailedTaskInfo::task_uuid);
