#ifndef TASKWARRIORREFERENCEDIALOG_HPP
#define TASKWARRIORREFERENCEDIALOG_HPP

#include <QDialog>
#include <QVariant>

class TaskwarriorReferenceDialog : public QDialog {
  public:
    explicit TaskwarriorReferenceDialog(QWidget *parent = nullptr);
    ~TaskwarriorReferenceDialog() override;

  private:
    void initUI();
};

#endif // TASKWARRIORREFERENCEDIALOG_HPP
