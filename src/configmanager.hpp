#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <map>
#include <variant>

#include <QString>
#include <QStringList>

class ConfigManager {
  public:
    template <typename taValueType>
    struct Key {
        QString name;
    };
    static inline const Key<QString> TaskBin{ "task_bin" };
    static inline const Key<bool> ShowTaskShell{ "show_task_shell" };
    static inline const Key<bool> HideWindowOnStartup{ "hide_on_startup" };
    static inline const Key<bool> SaveFilterOnExit{ "save_filter_on_exit" };
    static inline const Key<QStringList> TaskFilter{ "task_filter" };
    static inline const Key<bool> MuteNotifications{ "mute_notifications" };

    static ConfigManager &config();

    template <typename taValueType>
    [[nodiscard]] const taValueType &get(const Key<taValueType> &key) const
    {
        return std::get<taValueType>(m_named_fields_.at(key.name));
    }

    template <typename taValueType>
    void set(const Key<taValueType> &key, const taValueType &val)
    {
        m_named_fields_[key.name] = val;
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
};

#endif // CONFIGMANAGER_HPP
