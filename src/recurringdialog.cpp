#include "recurringdialog.hpp"

#include <QCoreApplication>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>

#include "recurringtasksmodel.hpp"

RecurringDialog::RecurringDialog(const QList<RecurringTask> &tasks,
                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QCoreApplication::applicationName() + " - Recurring tasks");
    setMinimumSize(500, 400);
    initUI(tasks);
}

RecurringDialog::~RecurringDialog() {}

void RecurringDialog::initUI(const QList<RecurringTask> &tasks)
{
    setWindowIcon(QIcon(":/icons/jtask.svg"));

    m_tasks_view = new QTableView(this);
    m_tasks_view->setShowGrid(true);

    m_tasks_view->verticalHeader()->setVisible(false);
    m_tasks_view->horizontalHeader()->setStretchLastSection(true);
    m_tasks_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    RecurringTasksModel *model = new RecurringTasksModel();
    model->setTasks(tasks);
    m_tasks_view->setModel(model);

    m_btn_box =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_btn_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(m_tasks_view);
    main_layout->addWidget(m_btn_box);
    main_layout->setContentsMargins(5, 5, 5, 5);

    setLayout(main_layout);
}
