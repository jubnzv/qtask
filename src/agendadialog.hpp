#ifndef AGENDADIALOG_HPP
#define AGENDADIALOG_HPP

#include <QCalendarWidget>
#include <QDialog>
#include <QListWidget>

#include "task.hpp"

class AgendaDialog : public QDialog {
    Q_OBJECT

  public:
    explicit AgendaDialog(QList<Task>, QWidget *parent = nullptr);
    ~AgendaDialog() override;

  private:
    void initUI();
    void setCalendarHighlight();

  private slots:
    void onUpdateTasks();

  private:
    QCalendarWidget *m_calendar;
    QListWidget *m_sched_tasks_list;
    QListWidget *m_due_tasks_list;
    QList<Task> m_tasks;
};

#endif // AGENDADIALOG_HPP
