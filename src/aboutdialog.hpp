#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>
#include <QVariant>
#include <QWidget>

class AboutDialog final : public QDialog {
  public:
    explicit AboutDialog(const QVariant &task_version,
                         QWidget *parent = nullptr);
    ~AboutDialog() final;

  private:
    void initUI(const QVariant &task_version);
};

#endif // ABOUTDIALOG_HPP
