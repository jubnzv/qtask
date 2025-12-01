#include "taskwarriorexecutor.hpp"

#include "date_time_parser.hpp"

#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QStringLiteral>

#include <iostream>
#include <stdexcept>
#include <utility>

namespace
{
/// @brief Executes @p binary and @returns TExecResult.
/// @note Empty lines are removed from result.
[[nodiscard]]
TaskWarriorExecutor::TExecResult execProgram(const QString &binary,
                                             const QStringList &all_params)
{
    constexpr auto kSplitBehaviour = DateTimeParser::kSplitSkipEmptyParts;
    constexpr int kStartDelayMs = 1000;
    constexpr int kFinishDelayMs = 30000;

    QProcess proc;
    proc.start(binary, all_params);

    if (!proc.waitForStarted(kStartDelayMs)) {
        return { TaskWarriorExecutor::TExecError{
            -1, QString("Failed to start: [ %1 ].").arg(proc.errorString()) } };
    }

    if (!proc.waitForFinished(kFinishDelayMs)) {
        return { TaskWarriorExecutor::TExecError{
            -1,
            QString("Execution timeout after %1ms.").arg(kFinishDelayMs) } };
    }

    const int exitCode = proc.exitCode();
    if (proc.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        return { TaskWarriorExecutor::TExecError{
            exitCode != 0 ? exitCode : -1,
            QString::fromLocal8Bit(proc.readAllStandardError()) } };
    }

    static const QRegularExpression kSplitter("[\r\n]");
    return TaskWarriorExecutor::TExecResult{
        QString::fromLocal8Bit(proc.readAllStandardOutput())
            .split(kSplitter, kSplitBehaviour)
    };
}
} // namespace

TaskWarriorExecutor::TaskWarriorExecutor(QString full_path_to_binary)
    : m_full_path_to_binary(std::move(full_path_to_binary))
{
    const static auto throw_if_true = [this](bool is_throw) {
        if (is_throw) {
            throw std::runtime_error(
                QStringLiteral(
                    "Given path %1 does not point to valid task binary.")
                    .arg(m_full_path_to_binary)
                    .toStdString());
        }
    };

    const auto version_res = execTaskProgram({ "version" });
    const auto &ver_strings = version_res.getStdout();
    throw_if_true(!version_res || ver_strings.size() < 1);
    m_task_version =
        DateTimeParser::splitSpaceSeparatedString(ver_strings.at(0)).at(1);
    throw_if_true(m_task_version.isEmpty());
    throw_if_true(!execTaskProgram({ "rc.gc=on" }));
}

const QString &TaskWarriorExecutor::getTaskVersion() const
{
    return m_task_version;
}

TaskWarriorExecutor::TExecResult
TaskWarriorExecutor::execTaskProgram(const QStringList &all_params) const
{
    qDebug() << m_full_path_to_binary << " " << all_params;
    auto res = execProgram(m_full_path_to_binary, all_params);
    qDebug() << res;
    if (!res) {
        std::cerr << "Error executing " << m_full_path_to_binary.toStdString()
                  << "\n";
        std::cerr << "Code: " << res.getError().code << ". "
                  << res.getError().message.toStdString() << std::endl;
    }
    return res;
}

TaskWarriorExecutor::TExecResult
TaskWarriorExecutor::execTaskProgramWithDefaults(
    const QStringList &all_params) const
{
    QStringList args{ "rc.gc=off", "rc.confirmation=off", "rc.bulk=0",
                      "rc.defaultwidth=0" };
    args << all_params;
    return execTaskProgram(args);
}
