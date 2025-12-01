#ifndef AGENDADIALOG_HPP
#define AGENDADIALOG_HPP

#include <QCalendarWidget>
#include <QDialog>
#include <QList>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>

#include "task.hpp"

class AgendaDialog : public QDialog {
    Q_OBJECT

  public:
    explicit AgendaDialog(QList<DetailedTaskInfo>, QWidget *parent = nullptr);
    ~AgendaDialog() override;

  private:
    void initUI();
    void setCalendarHighlight();

  private slots:
    void onUpdateTasks();

  private:
    QVBoxLayout *CreateLabeledVerticalLayout(const QString &label_text,
                                             QWidget *widget);

    QCalendarWidget *const m_calendar;
    QListWidget *const m_sched_tasks_list;
    QListWidget *const m_due_tasks_list;
    QList<DetailedTaskInfo> m_tasks;
};

#endif // AGENDADIALOG_HPP
