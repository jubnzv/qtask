#ifndef TASKSVIEW_HPP
#define TASKSVIEW_HPP

#include <QMouseEvent>
#include <QString>
#include <QTableView>

class TasksView : public QTableView {
    Q_OBJECT

  public:
    explicit TasksView(QWidget *parent = nullptr);
    ~TasksView() = default;

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  signals:
    void pushProjectFilter(const QString &);
};

#endif // TASKSVIEW_HPP
