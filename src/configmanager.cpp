#include "configmanager.hpp"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

#include <memory>

namespace {
const QString s_default_task_bin = "/usr/bin/task";
const QString s_default_task_data_path =
    QString("%1%2%3%2").arg(QDir::homePath(), QDir::separator(), ".task");
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_is_new(false)
    , m_config_path("")
    , m_task_bin(s_default_task_bin)
    , m_task_data_path(s_default_task_data_path)
    , m_show_task_shell(false)
    , m_hide_on_startup(false)
    , m_save_filter_on_exit(false)
    , m_task_filter({})
{
}

ConfigManager& ConfigManager::config()
{
    static std::unique_ptr<ConfigManager> lazy_single_inst(new ConfigManager());
    return *lazy_single_inst;
}

bool ConfigManager::initializeFromFile()
{
    // QSettings ini(QString("%1%2%1").arg(arch), QSettings::IniFormat);

    // Try to locate existing config directory
    QVariant existsting_dir;
    foreach (auto const &p, QStandardPaths::standardLocations(
                                QStandardPaths::ConfigLocation)) {
        QDir d(p);
        if (d.exists("qtask")) {
            existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
            break;
        }
    }

    // Try to initialize a new config directory, if not found
    if (existsting_dir.isNull()) {
        foreach (auto const &p, QStandardPaths::standardLocations(
                                    QStandardPaths::ConfigLocation)) {
            QDir d(p);
            if (d.mkdir("qtask")) {
                existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
                break;
            }
        }
    }

    if (existsting_dir.isNull()) {
        return false;
    }

    QDir config_dir(existsting_dir.toString());
    m_config_path = config_dir.absoluteFilePath("qtask.ini");
    if (!config_dir.exists("qtask.ini")) {
        if (!createNewConfigFile())
            return false;
        m_is_new = true;
        return true;
    }

    return fillOptionsFromConfigFile();
}

void ConfigManager::updateConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable())
        return;
    settings.setValue("task_bin", m_task_bin);
    settings.setValue("task_data_path", m_task_data_path);
    settings.setValue("show_task_shell", m_show_task_shell);
    settings.setValue("hide_on_startup", m_hide_on_startup);
    settings.setValue("save_filter_on_exit", m_save_filter_on_exit);
    settings.setValue("task_filter", m_task_filter);
}

bool ConfigManager::createNewConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable())
        return false;
    settings.setValue("task_bin", s_default_task_bin);
    settings.setValue("task_data_path", s_default_task_data_path);
    settings.setValue("show_task_shell", false);
    settings.setValue("hide_on_startup", false);
    settings.setValue("save_filter_on_exit", false);
    settings.setValue("task_filter", {});

    return true;
}

bool ConfigManager::fillOptionsFromConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    m_task_bin = settings.value("task_bin", s_default_task_bin).toString();
    m_task_data_path =
        settings.value("task_data_path", s_default_task_data_path).toString();
    m_show_task_shell = settings.value("show_task_shell", false).toBool();
    m_hide_on_startup = settings.value("hide_on_startup", false).toBool();
    m_save_filter_on_exit =
        settings.value("save_filter_on_exit", false).toBool();
    m_task_filter = settings.value("task_filter", "").toStringList();
    return true;
}
