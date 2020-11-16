#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>
#include <QVariant>

class AboutDialog : public QDialog {
  public:
    explicit AboutDialog(const QVariant &task_version,
                         QWidget *parent = nullptr);
    ~AboutDialog() = default;

  private:
    void initUI(const QVariant &task_version);
};

#endif // ABOUTDIALOG_HPP
