#ifndef SETTINGSDIALOG_HPP
#define SETTINGSDIALOG_HPP

#include <QAbstractButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>

class SettingsDialog : public QDialog {
    Q_OBJECT

  public:
    SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

  private:
    void initUI();
    void applySettings();

  private slots:
    void onButtonBoxClicked(QAbstractButton *);

  private:
    QLineEdit *m_task_bin_edit;
    QLineEdit *m_task_data_path_edit;
    QDialogButtonBox *m_buttons;
};

#endif // SETTINGSDIALOG_HPP
