#ifndef ABOUTDIALOG_HPP
#define ABOUTDIALOG_HPP

#include <QDialog>

class AboutDialog : public QDialog {
  public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() = default;

  private:
    void initUI();
};

#endif // ABOUTDIALOG_HPP
