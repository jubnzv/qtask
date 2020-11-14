#include "configmanager.hpp"

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QString>

ConfigManager *ConfigManager::inst_ = nullptr;

const QString ConfigManager::s_default_task_bin = "/usr/bin/task";
const QString ConfigManager::s_default_task_data_path =
    QString("%1%2%3%2").arg(QDir::homePath(), QDir::separator(), ".task");

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_is_new(false)
    , m_config_path("")
    , m_task_bin(s_default_task_bin)
    , m_task_data_path(s_default_task_data_path)
{
}

ConfigManager *ConfigManager::config()
{
    if (inst_ == nullptr)
        inst_ = new ConfigManager();
    return (inst_);
}

bool ConfigManager::initializeFromFile()
{
    // QSettings ini(QString("%1%2%1").arg(arch), QSettings::IniFormat);

    // Try to locate existing config directory
    QVariant existsting_dir;
    foreach (auto const &p, QStandardPaths::standardLocations(
                                QStandardPaths::ConfigLocation)) {
        QDir d(p);
        if (d.exists(".jtask")) {
            existsting_dir = QString("%1%2jtask").arg(p, QDir::separator());
            break;
        }
    }

    // Try to initialize a new config directory, if not found
    if (existsting_dir.isNull()) {
        foreach (auto const &p, QStandardPaths::standardLocations(
                                    QStandardPaths::ConfigLocation)) {
            QDir d(p);
            if (d.mkdir("jtask")) {
                existsting_dir = QString("%1%2jtask").arg(p, QDir::separator());
                break;
            }
        }
    }

    if (existsting_dir.isNull()) {
        return false;
    }

    QDir config_dir(existsting_dir.toString());
    m_config_path = config_dir.absoluteFilePath("jtask.ini");
    if (!config_dir.exists("jtask.ini")) {
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
    settings.setValue("task_bin", m_task_bin);
    settings.setValue("task_data_path", m_task_data_path);
}

bool ConfigManager::createNewConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable())
        return false;
    settings.setValue("task_bin", s_default_task_bin);
    settings.setValue("task_data_path", s_default_task_data_path);
    return true;
}

bool ConfigManager::fillOptionsFromConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    m_task_bin = settings.value("task_bin", s_default_task_bin).toString();
    m_task_data_path =
        settings.value("task_data_path", s_default_task_data_path).toString();
    return true;
}
