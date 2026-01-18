#include "configmanager.hpp"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <type_traits>
#include <variant>

ConfigManager::ConfigManager()
    : m_config_path("")
    , m_named_fields_({
          { TaskBin.name, QString("/usr/bin/task") },
          { ShowTaskShell.name, false },
          { HideWindowOnStartup.name, false },
          { SaveFilterOnExit.name, false },
          { TaskFilter.name, QStringList{} },
          { MuteNotifications.name, false },
      })
    , m_named_fields_defaults_(m_named_fields_)
{
}

ConfigManager &ConfigManager::config()
{
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::initializeFromFile()
{
    // QSettings ini(QString("%1%2%1").arg(arch), QSettings::IniFormat);

    // Try to locate existing config directory
    QVariant existsting_dir;
    for (const auto &p :
         QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)) {
        QDir const d(p);
        if (d.exists("qtask")) {
            existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
            break;
        }
    }

    // Try to initialize a new config directory, if not found
    if (existsting_dir.isNull()) {
        for (const auto &p : QStandardPaths::standardLocations(
                 QStandardPaths::ConfigLocation)) {
            QDir const d(p);
            if (d.mkdir("qtask")) {
                existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
                break;
            }
        }
    }

    if (existsting_dir.isNull()) {
        return false;
    }

    QDir const config_dir(existsting_dir.toString());
    m_config_path = config_dir.absoluteFilePath("qtask.ini");
    if (!config_dir.exists("qtask.ini")) {
        m_named_fields_ = m_named_fields_defaults_;
        return updateConfigFile();
    }

    return fillOptionsFromConfigFile();
}

bool ConfigManager::updateConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable()) {
        return false;
    }
    for (const auto &[name, field_variant] : m_named_fields_) {
        std::visit([&name, &settings](
                       const auto &value) { settings.setValue(name, value); },
                   field_variant);
    }
    return true;
}

bool ConfigManager::fillOptionsFromConfigFile()
{
    const QSettings settings(m_config_path, QSettings::IniFormat);

    for (auto &[name, field_variant] : m_named_fields_) {
        const QVariant def =
            std::visit([](const auto &v) { return QVariant::fromValue(v); },
                       m_named_fields_defaults_.at(name));
        QVariant val = settings.value(name, def);
        std::visit(
            [&val](auto &typed_value) {
                using T = std::decay_t<decltype(typed_value)>;
                typed_value = val.value<T>();
            },
            field_variant);
    }
    return true;
}
