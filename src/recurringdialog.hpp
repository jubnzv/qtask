#ifndef RECURRINGDIALOG_HPP
#define RECURRINGDIALOG_HPP

#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QObject>
#include <QTableView>
#include <QWidget>

#include "task.hpp"

class RecurringDialog : public QDialog {
    Q_OBJECT

  public:
    explicit RecurringDialog(QList<RecurringTask> tasks,
                             QWidget *parent = nullptr);
    ~RecurringDialog() override;

  private:
    void initUI(QList<RecurringTask> tasks);

  private:
    QTableView *m_tasks_view;
    QDialogButtonBox *m_btn_box;
};

#endif // RECURRINGDIALOG_HPP
