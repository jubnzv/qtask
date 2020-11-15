#include "filterlineedit.hpp"

#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSize>
#include <QVariant>

FilterLineEdit::FilterLineEdit(QWidget *parent)
    : QLineEdit(parent)
    , m_leading_offset(QVariant())
{
    setMinimumHeight(24);
}

FilterLineEdit::~FilterLineEdit() {}

QStringList FilterLineEdit::getTags() const { return m_tags; }

QString FilterLineEdit::getTagsStr() const { return m_tags.join(' '); }

void FilterLineEdit::setTags(const QString &tags)
{
    if (!tags.isEmpty()) {
        m_tags = tags.split(' ');
        clear();
        update();
        emit tagsChanged();
    }
}

void FilterLineEdit::setTags(const QStringList &tags)
{
    if (tags.isEmpty())
        return;
    m_tags = tags;
    clear();
    update();
    emit tagsChanged();
}

void FilterLineEdit::pushTag(const QString &value)
{
    m_tags.push_back(value);
    clear();
    update();
    emit tagsChanged();
}

void FilterLineEdit::popTag()
{
    if (m_tags.isEmpty())
        return;
    m_tags.pop_back();
    clear();
    update();
    emit tagsChanged();
}

void FilterLineEdit::addTag()
{
    if (text().isEmpty())
        return;
    m_tags.append(text());
    clear();
    update();
}

void FilterLineEdit::leaveEvent(QEvent *evt)
{
    QLineEdit::leaveEvent(evt);
    m_cursor_pos = m_clicked_pos = QPoint();
}

void FilterLineEdit::keyPressEvent(QKeyEvent *evt)
{
    if (evt->key() == Qt::Key_Return || evt->key() == Qt::Key_Enter ||
        evt->key() == Qt::Key_Space) {
        evt->accept();
        addTag();
        emit tagsChanged();
    } else if (evt->key() == Qt::Key_Backspace && !m_tags.empty()) {
        evt->accept();
        popTag();
        emit tagsChanged();
    } else {
        QLineEdit::keyPressEvent(evt);
    }
}

void FilterLineEdit::mousePressEvent(QMouseEvent *evt)
{
    m_clicked_pos = evt->pos();
    QLineEdit::mousePressEvent(evt);
}

void FilterLineEdit::mouseMoveEvent(QMouseEvent *evt)
{
    QLineEdit::mouseMoveEvent(evt);
    m_cursor_pos = evt->pos();
}

void FilterLineEdit::paintEvent(QPaintEvent *evt)
{
    QLineEdit::paintEvent(evt);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QFontMetrics fm = fontMetrics();
    QRectF r = rect();
    double space = double(r.height() - fm.height()) / 2;
    constexpr int padding = 4;
    const int icon_size = m_leading_offset.toInt();
    int cross_size = int(r.height() - space * 2);
    r.setX(padding + icon_size);
    r.setY(double(r.height() - fm.height()) / 4);
    QPen pen = painter.pen();
    QBrush brush = painter.brush();
    QColor tag_bg(225, 236, 244);
    QColor tag_txt(44, 87, 119);
    int x_offset = padding + icon_size;
    int idx = 0;

    for (const QString &tag : m_tags) {
        QRect tag_rect = fm.boundingRect(tag);
        r.setWidth(tag_rect.width() + (padding * 2) + (padding + cross_size));
        r.setHeight(tag_rect.height() + space);
        QRectF cross_rect = QRectF(r.x() + r.width() - cross_size - padding,
                                   space, cross_size, cross_size);
        if (cross_rect.contains(m_clicked_pos)) {
            m_clicked_pos = QPoint();
            m_tags.removeAt(idx);
            emit tagsChanged();
            continue;
        }

        QPainterPath tag_path;
        tag_path.addRoundedRect(r, 2, 2);

        // Draw tag background and text
        painter.fillPath(tag_path, QBrush(tag_bg));
        painter.setPen(tag_txt);
        painter.drawText(QRectF(r.x() + padding, 0, tag_rect.width(), height()),
                         Qt::AlignVCenter, tag);

        // Draw the cross to remove tag
        bool is_hovered = r.contains(m_cursor_pos);
        if (is_hovered) {
            // Manage the hovered
            QPainterPath cross_path;
            cross_path.addRoundedRect(cross_rect, 2, 2);
            painter.fillPath(cross_path, QBrush(tag_txt));
            // Manage the remove tag mouse cursor
            if (cross_rect.contains(m_cursor_pos)) {
                setCursor(Qt::PointingHandCursor);
            } else {
                setCursor(Qt::ArrowCursor);
            }
        }
        pen.setWidth(2);
        pen.setColor((is_hovered ? tag_bg : tag_txt));
        painter.setPen(pen);
        QPointF top_l = cross_rect.topLeft();
        QPointF top_r = cross_rect.topRight();
        QPointF bot_l = cross_rect.bottomLeft();
        QPointF bot_r = cross_rect.bottomRight();
        top_l.setX(top_l.x() + 3);
        top_l.setY(top_l.y() + 3);
        top_r.setX(top_r.x() - 3);
        top_r.setY(top_r.y() + 3);
        bot_l.setX(bot_l.x() + 3);
        bot_l.setY(bot_l.y() - 3);
        bot_r.setX(bot_r.x() - 3);
        bot_r.setY(bot_r.y() - 3);
        painter.drawLine(top_l, bot_r);
        painter.drawLine(top_r, bot_l);

        x_offset += r.width() + padding;
        r.setX(x_offset);
        idx++;
    }

    if (textMargins().left() != (x_offset)) {
        if (m_tags.isEmpty()) {
            setTextMargins(0, 0, 0, 0);
        } else {
            setTextMargins(x_offset - padding - icon_size, 0, 0, 0);
        }
    }
}
