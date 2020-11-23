#include "settingsdialog.hpp"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>

#include "configmanager.hpp"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    initUI();
}

SettingsDialog::~SettingsDialog() {}

void SettingsDialog::initUI()
{
    QGridLayout *main_layout = new QGridLayout();
    main_layout->setContentsMargins(5, 5, 5, 5);

    setWindowTitle(QCoreApplication::applicationName() + " - Settings");
    setWindowIcon(QIcon(":/icons/qtask.svg"));

    QLabel *task_bin_label = new QLabel("task executable:");
    main_layout->addWidget(task_bin_label, 0, 0);

    m_task_bin_edit = new QLineEdit();
    m_task_bin_edit->setText(ConfigManager::config()->getTaskBin());
    main_layout->addWidget(m_task_bin_edit, 0, 1);

    QLabel *task_data_path_label = new QLabel("Path to task data:");
    main_layout->addWidget(task_data_path_label, 1, 0);

    m_task_data_path_edit = new QLineEdit();
    m_task_data_path_edit->setText(ConfigManager::config()->getTaskDataPath());
    main_layout->addWidget(m_task_data_path_edit, 1, 1);

    m_hide_on_startup_cb = new QCheckBox(tr("Hide QTask window on startup"));
    m_hide_on_startup_cb->setChecked(
        ConfigManager::config()->getHideWindowOnStartup());
    main_layout->addWidget(m_hide_on_startup_cb, 2, 0, 1, 2);

    m_buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply |
                             QDialogButtonBox::Close);
    connect(m_buttons, &QDialogButtonBox::clicked, this,
            &SettingsDialog::onButtonBoxClicked);
    main_layout->addWidget(m_buttons, 3, 0, 1, 2);

    setLayout(main_layout);
}

void SettingsDialog::applySettings()
{
    auto task_data_path = m_task_data_path_edit->text();
    if (ConfigManager::config()->getTaskDataPath() != task_data_path)
        ConfigManager::config()->setTaskDataPath(task_data_path);

    auto task_bin = m_task_bin_edit->text();
    if (ConfigManager::config()->getTaskBin() != task_bin)
        ConfigManager::config()->setTaskBin(task_bin);

    ConfigManager::config()->setHideWindowOnStartup(
        m_hide_on_startup_cb->isChecked());

    ConfigManager::config()->updateConfigFile();
}

void SettingsDialog::onButtonBoxClicked(QAbstractButton *button)
{
    switch (m_buttons->standardButton(button)) {
    case QDialogButtonBox::Apply:
        applySettings();
        break;
    case QDialogButtonBox::Ok:
        applySettings();
        close();
    default:
        close();
    }
}
