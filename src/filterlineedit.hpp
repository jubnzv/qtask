#ifndef FILTERLINEEDIT_HPP
#define FILTERLINEEDIT_HPP

#include <QLineEdit>

class FilterLineEdit final : public QLineEdit {
    Q_OBJECT

    Q_PROPERTY(QString tags READ getTagsStr WRITE setTags)

  public:
    explicit FilterLineEdit(QWidget *parent = nullptr);
    ~FilterLineEdit() override;
    QStringList getTags() const;
    QString getTagsStr() const;
    void setTags(const QString &tags);
    void setTags(const QStringList &tags);
    void popTag();

    void setLeadingOffset(const int &v) { m_leading_offset = { v }; }

  private slots:
    void addTag();

  protected:
    void leaveEvent(QEvent *evt) override;
    void keyPressEvent(QKeyEvent *evt) override;
    void mousePressEvent(QMouseEvent *evt) override;
    void mouseMoveEvent(QMouseEvent *evt) override;
    void paintEvent(QPaintEvent *evt) override;

  private:
    bool hasLeadingOffset() const { return m_leading_offset.isNull(); }

  signals:
    void tagsChanged();

  private:
    QStringList m_tags;
    QPoint m_cursor_pos, m_clicked_pos;
    QVariant m_leading_offset;
};

#endif // FILTERLINEEDIT_HPP
