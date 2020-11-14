#ifndef DATETIMEDIALOG_HPP
#define DATETIMEDIALOG_HPP

#include <QCalendarWidget>
#include <QDateTimeEdit>
#include <QDialog>

class DateTimeDialog : public QDialog {
    Q_OBJECT

  public:
    DateTimeDialog(QWidget *parent = nullptr);
    DateTimeDialog(const QDateTime &, QWidget *parent = nullptr);
    ~DateTimeDialog();

    QDateTime getDateTime() const;

  private:
    void initUI();

  private:
    QDateTimeEdit *m_datetime_edit;
    QCalendarWidget *m_calendar;

    QDateTime m_datetime;
};

#endif // DATETIMEDIALOG_HPP
