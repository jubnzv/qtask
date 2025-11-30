#pragma once

#include <QString>
#include <QStringList>

#include <variant>

/// @brief This class provides generic interface to launch `task`command.
class TaskWarriorExecutor {
  public:
    /// @brief Error code and error message of executing the program.
    struct TExecError {
        int code;
        QString message;
    };

    /// @brief It is resulting error code or stdout of calling external binary.
    struct TExecResult {
        std::variant<TExecError, QStringList> exec_result_value;
        static constexpr int kSuccessCode = 0;

        [[nodiscard]]
        const TExecError &getError() const
        {
            static const TExecError kNoError{ kSuccessCode, {} };

            if (const TExecError *error =
                    std::get_if<TExecError>(&exec_result_value)) {
                return *error;
            }
            return kNoError;
        }

        [[nodiscard]]
        const QStringList &getStdout() const
        {
            static const QStringList kNoStdout;

            if (const QStringList *stdout_lines =
                    std::get_if<QStringList>(&exec_result_value)) {
                return *stdout_lines;
            }
            return kNoStdout;
        }

        /// @brief This object evaluates to true in bool context if it has no
        /// error.
        /// @returns true if it was NO error.
        [[nodiscard]]
        operator bool() const // NOLINT
        {
            return getError().code == kSuccessCode;
        }
    };

    TaskWarriorExecutor() = delete;

    /// @brief Constructs object with given path.
    /// @param full_path_to_binary should point to binary `task`.
    /// @throws if given path does not point to proper `task` command.
    explicit TaskWarriorExecutor(QString full_path_to_binary);

    /// @brief Executes configured task with defaults required parameters and
    /// @returns TExecResult.
    [[nodiscard]]
    TExecResult
    execTaskProgramWithDefaults(const QStringList &all_params) const;

    /// @returns `task` version detected.
    [[nodiscard]]
    const QString &getTaskVersion() const;

  protected:
    /// @brief Executes configured "task" program and @returns TExecResult.
    [[nodiscard]]
    TExecResult execTaskProgram(const QStringList &all_params) const;

  private:
    QString m_full_path_to_binary;
    QString m_task_version;
};
