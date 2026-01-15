#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <map>
#include <type_traits>
#include <variant>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

class ConfigEvents : public QObject {
    Q_OBJECT
  signals:
    void settingsChanged();
};

class ConfigManager {
  public:
    template <typename taValueType>
    struct Key {
        using value_type = taValueType;
        QString name;
    };

    using KeyVariant = std::variant<Key<bool>, Key<QString>, Key<QStringList>>;

    static inline const Key<QString> TaskBin{ "task_bin" };
    static inline const Key<bool> ShowTaskShell{ "show_task_shell" };
    static inline const Key<bool> HideWindowOnStartup{ "hide_on_startup" };
    static inline const Key<bool> SaveFilterOnExit{ "save_filter_on_exit" };
    static inline const Key<QStringList> TaskFilter{ "task_filter" };
    static inline const Key<bool> MuteNotifications{ "mute_notifications" };

    static ConfigManager &config();
    ConfigEvents &notifier() { return m_events_; }

    template <typename taValueType>
    [[nodiscard]] const taValueType &get(const Key<taValueType> &key) const
    {
        return std::get<taValueType>(m_named_fields_.at(key.name));
    }

    [[nodiscard]] QVariant get_as_qvariant(const KeyVariant &key) const
    {
        return std::visit(
            [this](const auto &k) -> QVariant {
                return QVariant::fromValue(this->get(k));
            },
            key);
    }

    template <typename taValueType>
    void set(const Key<taValueType> &key, const taValueType &val)
    {
        auto &current = std::get<taValueType>(m_named_fields_.at(key.name));
        if (current != val) {
            current = val;
            emit m_events_.settingsChanged();
        }
    }

    void set_qvariant(const KeyVariant &key, const QVariant &val)
    {
        std::visit(
            [this, &val](const auto &k) {
                using T = typename std::decay_t<decltype(k)>::value_type;
                this->set(k, val.value<T>());
            },
            key);
    }

    [[nodiscard]]
    bool initializeFromFile();
    bool updateConfigFile();

  private:
    using TSettingType = std::variant<bool, QString, QStringList>;
    ConfigManager();
    bool fillOptionsFromConfigFile();

    /// Path to configuration file
    QString m_config_path;

    std::map<QString, TSettingType> m_named_fields_;
    const std::map<QString, TSettingType> m_named_fields_defaults_;
    ConfigEvents m_events_;
};

#endif // CONFIGMANAGER_HPP
