#ifndef TAGSEDIT_HPP
#define TAGSEDIT_HPP

#include <QWidget>

#include <memory>

class QStyleOptionFrame;

/// Tag editor widget
/// `Space` commits a tag and initiates a new tag edition
class TagsEdit : public QWidget {
    Q_OBJECT

  public:
    explicit TagsEdit(QWidget *parent = nullptr);
    ~TagsEdit() override;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /// Set completions
    void completion(std::vector<QString> const &completions);

    void setTags(const QStringList &tags);
    void clearTags();
    QStringList getTags() const;
    void pushTag(const QString &);
    void popTag();

  signals:
    void tagsChanged();

  protected:
    // QWidget
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

#endif // TAGSEDIT_HPP
