#include "recurringdialog.hpp"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QIcon>
#include <QList>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

#include "recurringtasksmodel.hpp"
#include "task.hpp"

#include <utility>

RecurringDialog::RecurringDialog(QList<RecurringTask> tasks, QWidget *parent)
    : QDialog(parent)
    , m_tasks_view(new QTableView(this))
    , m_btn_box(new QDialogButtonBox(
          QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this))
{
    setWindowTitle(QCoreApplication::applicationName() + " - Recurring tasks");
    setMinimumSize(500, 400);
    initUI(std::move(tasks));
}

RecurringDialog::~RecurringDialog() = default;

void RecurringDialog::initUI(QList<RecurringTask> tasks)
{
    setWindowIcon(QIcon(":/icons/qtask.svg"));
    m_tasks_view->setShowGrid(true);
    m_tasks_view->verticalHeader()->setVisible(false);
    m_tasks_view->horizontalHeader()->setStretchLastSection(true);
    m_tasks_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *model = new RecurringTasksModel();
    model->setTasks(std::move(tasks));
    m_tasks_view->setModel(model);

    connect(m_btn_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *main_layout = new QVBoxLayout();
    main_layout->addWidget(m_tasks_view);
    main_layout->addWidget(m_btn_box);
    main_layout->setContentsMargins(5, 5, 5, 5);

    setLayout(main_layout);
}
