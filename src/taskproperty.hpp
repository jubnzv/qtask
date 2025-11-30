#pragma once

#include <QString>
#include <QStringList>

#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

#include "lambda_visitors.hpp"

template <typename T, typename = void>
struct is_arg_compatible : std::false_type {};

template <typename T>
struct is_arg_compatible<T, std::void_t<decltype(std::declval<QString>().arg(
                                std::declval<const T &>()))>> : std::true_type {
};

/// @brief This is some property of the task.
/// It binds together actual value and a way to write it to taskwarriror.
/// Also it tracks if it was changed, so only modified fields could be updated.
/// It can use provided format string or functional formatter.
/// @note It is NOT thread safe.
template <typename taStoredType>
class TaskProperty {
  public:
    using FormatterToSingleStr = std::function<QString(const taStoredType &)>;
    using FormatterToManyStr = std::function<QStringList(const taStoredType &)>;

    using Formatter =
        std::variant<QString, FormatterToSingleStr, FormatterToManyStr>;

    explicit TaskProperty(Formatter formatter,
                          taStoredType value = taStoredType())
        : m_value(std::move(value))
        , m_formater{ std::move(formatter) }
    {
    }

    template <typename TValue>
    TaskProperty &operator=(TValue &&value)
    {
        modify([&value](taStoredType &target) {
            target = std::forward<TValue>(value);
        });
        return *this;
    }

    /// @brief executes @p callback passing to it stored value, @p callback
    /// should accept it by reference and change as needed. Sets "modified"
    /// flag.
    /// @param callback should be like: void func(taStoredType&)
    /// @note "modified" flag is set always once this method call ended.
    template <typename taCallable>
    void modify(const taCallable &callback)
    {
        callback(m_value);
        m_modified = true;
    }

    [[nodiscard]]
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator const taStoredType &() const
    {
        return get();
    }

    [[nodiscard]]
    const taStoredType &get() const
    {
        return m_value;
    }

    /// @returns true if property was modified.
    [[nodiscard]]
    bool isModified() const
    {
        return m_modified;
    }

    /// @brief Clears modification flag, so property is considered "not set".
    void setNotModified() { m_modified = false; }

    /// @brief Converts property for command line using formatters provided to
    /// ctor.
    /// @returns QStringList usable to direct appending to command line call.
    [[nodiscard]]
    QStringList getStringsForCmd() const
    {
        const LambdaVisitor visitors = {
            [this](const QString &format) {
                if constexpr (is_arg_compatible<taStoredType>::value) {
                    return QStringList() << format.arg(m_value);
                } else {
                    assert(false &&
                           "taStoredType is incompatible with QString::arg and "
                           "cannot be formatted via QString pattern.");
                    return QStringList() << format;
                }
            },
            [this](const FormatterToSingleStr &format) {
                assert(format);
                if (!format) {
                    return QStringList{};
                }
                return QStringList() << format(m_value);
            },
            [this](const FormatterToManyStr &format) {
                assert(format);
                if (!format) {
                    return QStringList{};
                }
                return format(m_value);
            },
        };
        return std::visit(visitors, m_formater);
    }

  private:
    taStoredType m_value;
    Formatter m_formater;

    bool m_modified{ false };
};
