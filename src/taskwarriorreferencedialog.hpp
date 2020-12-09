#ifndef TASKWARRIORREFERENCEDIALOG_HPP
#define TASKWARRIORREFERENCEDIALOG_HPP

#include <QDialog>
#include <QVariant>

class TaskwarriorReferenceDialog : public QDialog {
  public:
    explicit TaskwarriorReferenceDialog(QWidget *parent = nullptr);
    ~TaskwarriorReferenceDialog() = default;

  private:
    void initUI();
};

#endif // TASKWARRIORREFERENCEDIALOG_HPP
