#include "tasksview.hpp"

#include <QDebug>
#include <QModelIndex>
#include <QMouseEvent>
#include <QTableView>

TasksView::TasksView(QWidget *parent)
    : QTableView(parent)
{
}

void TasksView::mousePressEvent(QMouseEvent *event)
{
    constexpr int project_column = 1;

    QModelIndex i = indexAt(event->pos());

    // Right click to "column" will add project value to taskwarrior filter
    if (i.column() == project_column && event->buttons() & Qt::RightButton) {
        auto d = i.data();
        if (!d.isNull())
            emit pushProjectFilter("pro:" + d.toString());
    }

    QTableView::mousePressEvent(event);
}
