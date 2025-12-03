#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QAbstractButton>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QWidget>

class SettingsDialog : public QDialog {
    Q_OBJECT

  public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

  private slots:
    void onButtonBoxClicked(QAbstractButton *);

  private:
    void initUI();
    void applySettings();

    QLineEdit *const m_task_bin_edit;
    QCheckBox *const m_hide_on_startup_cb;
    QCheckBox *const m_save_filter_on_exit;
    QDialogButtonBox *const m_buttons;
};

#endif // SETTINGSDIALOG_HPP
