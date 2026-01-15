#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include "configmanager.hpp"

#include <variant>

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QList>
#include <QWidget>

class SettingsDialog : public QDialog {
    Q_OBJECT

  public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

  private slots:
    void onButtonBoxClicked(QAbstractButton *);

  private:
    struct SettingBinder {
        using WidgetVariant = std::variant<QCheckBox *, QLineEdit *>;
        ConfigManager::KeyVariant key;
        WidgetVariant widget;
    };

    void initUI();
    void applySettings();
    void updateButtonsState();

    QLineEdit *const m_task_bin_edit;
    QCheckBox *const m_hide_on_startup_cb;
    QCheckBox *const m_save_filter_on_exit;
    QCheckBox *const m_mute_notifications_cb;
    QDialogButtonBox *const m_buttons;

    // Binds config keys with widgets.
    const QList<SettingBinder> binders;
};

#endif // SETTINGSDIALOG_HPP
