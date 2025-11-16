#ifndef DATETIMEDIALOG_HPP
#define DATETIMEDIALOG_HPP

#include <QCalendarWidget>
#include <QDateTimeEdit>
#include <QDialog>

class DateTimeDialog : public QDialog {
    Q_OBJECT

  public:
    DateTimeDialog(QWidget *parent = nullptr);
    DateTimeDialog(QDateTime, QWidget *parent = nullptr);
    ~DateTimeDialog() override;

    const QDateTime &getDateTime() const;

  private:
    QCalendarWidget *const m_calendar;
    QDateTimeEdit *const m_datetime_edit;
    QDateTime m_datetime;
};

#endif // DATETIMEDIALOG_HPP
