#ifndef RECURRINGDIALOG_HPP
#define RECURRINGDIALOG_HPP

#include <QDialog>
#include <QDialogButtonBox>
#include <QTableView>
#include <QWidget>

#include "recurringtasksmodel.hpp"
#include "task.hpp"

class RecurringDialog : public QDialog {
    Q_OBJECT

  public:
    RecurringDialog(const QList<RecurringTask> &, QWidget *parent = nullptr);
    ~RecurringDialog();

  private:
    void initUI(const QList<RecurringTask> &);

  private:
    QTableView *m_tasks_view;
    QDialogButtonBox *m_btn_box;
};

#endif // RECURRINGDIALOG_HPP
