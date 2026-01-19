#include "allatoncekeywordsfinder.h"

#include "taskwarriorexecutor.hpp"

#include <iostream>
#include <optional>
#include <utility>

AllAtOnceKeywordsFinder::AllAtOnceKeywordsFinder(QStringList keywords) // NOLINT
    : m_user_keywords(std::move(keywords))
{
}

bool AllAtOnceKeywordsFinder::readIds(const TaskWarriorExecutor &executor)
{
    m_ids = std::nullopt; // Didn't search for
    if (m_user_keywords.empty()) {
        return true;
    }
    const auto resp =
        executor.execTaskProgramWithDefaults(QStringList("ids") // NOLINT
                                             << m_user_keywords);

    if (resp) {
        const auto &stdOut = resp.getStdout();

        // Examples:
        // task ids Bank
        // 1-2
        // task ids 1 2 3 4 5 6 7 8 9 111
        // 1-6
        // task ids 2 5 1 4
        // 1-2 4-5

        if (stdOut.size() == 1) {
            m_ids = stdOut.first(); // Found something
            return true;
        }
        if (stdOut.size() == 0) {
            m_ids = QString{}; // Not Found
            return true;
        }
        std::cerr << "Unexpected result of ids command: "
                  << stdOut.join("\n").toStdString() << std::endl;
    }
    return false;
}
