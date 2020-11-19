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
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  signals:
    void pushProjectFilter(const QString &);
    void linkActivated(const QString &link);
    void linkHovered(const QString &link);
    void linkUnhovered();

  private:
    QString anchorAt(const QPoint &pos) const;

  private:
    QString m_mouse_press_anchor;
    QString m_last_hovered_anchor;
};

#endif // TASKSVIEW_HPP
