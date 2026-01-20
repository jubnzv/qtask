#ifndef RECURRINGDIALOG_HPP
#define RECURRINGDIALOG_HPP

#include <QDialog>
#include <QDialogButtonBox>
#include <QList>
#include <QObject>
#include <QTableView>
#include <QWidget>

#include "recurring_task_template.hpp"

class RecurringDialog : public QDialog {
    Q_OBJECT

  public:
    explicit RecurringDialog(QList<RecurringTaskTemplate> tasks,
                             QWidget *parent = nullptr);
    ~RecurringDialog() override;

  private:
    void initUI(QList<RecurringTaskTemplate> tasks);

  private:
    QTableView *const m_tasks_view;
    QDialogButtonBox *const m_btn_box;
};

#endif // RECURRINGDIALOG_HPP
