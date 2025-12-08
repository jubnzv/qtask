#include "tagsedit.hpp"

#include <QDebug>

#include <QApplication>
#include <QChar>
#include <QColor>
#include <QCompleter>
#include <QDebug>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMouseEvent>
#include <QOverload>
#include <QPainter>
#include <QPainterPath>
#include <QSizePolicy>
#include <QStringList>
#include <QStyle>
#include <QStyleHints>
#include <QTextLayout>
#include <QTimerEvent>
#include <QVector>
#include <QtGui/private/qinputcontrol_p.h>
#include <QtMinMax>
#include <QtNumeric>
#include <QtTypes>
#include <QtVersionChecks>
#include <qnamespace.h>
#include <qtmetamacros.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "tags_rects_calculator.h"
#include "visual_tag.hpp"

namespace
{

constexpr int vertical_margin = 3;
constexpr int bottommargin = 1;
constexpr int topmargin = 1;

constexpr int horizontal_margin = 3;
constexpr int leftmargin = 1;
constexpr int rightmargin = 1;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
QSize inferredGlobalStrut()
{
    static const QSize kDefault{ 8, 16 };
    // This function is AI suggested.
    const QFontMetrics fm(qApp->font());
    const int textH = fm.height();
    const int charW = fm.horizontalAdvance(QLatin1Char('M'));

    const QStyle *style = qApp->style();
    if (!style) {
        return kDefault;
    }
    const int frame = style->pixelMetric(QStyle::PM_DefaultFrameWidth);
    const int spacingH = style->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    const int spacingW = style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
    const int btnIcon = style->pixelMetric(QStyle::PM_ButtonIconSize);

    int minh = qMax(textH + spacingH + frame * 2, btnIcon);
    int minw = qMax(charW * 2 + spacingW + frame * 2, btnIcon);
    if (minh <= 0) {
        minh = kDefault.height();
    }
    if (minw <= 0) {
        minw = kDefault.width();
    }

    return { minw, minh };
}
#endif

QSize globalStrut()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    return QApplication::globalStrut();
#else
    return inferredGlobalStrut();
#endif
}

} // namespace

struct TagsEdit::Impl {
    explicit Impl(TagsEdit *const &ifce)
        : ifce(ifce)
    {
    }

    [[nodiscard]]
    bool inCrossArea(size_t tag_index, QPoint const &point) const
    {
        return TagsRectsCalculator::localCloseTagCrossRect(
                   tags[tag_index].rect())
                   .adjusted(-2, 0, 0, 0)
                   .translated(-hscroll, 0)
                   .contains(point) &&
               (!cursorVisible() || tag_index != editing_index);
    }

    template <typename ItA, typename ItB>
    void drawTags(QPainter &p, std::pair<ItA, ItB> range) const
    {
        for (auto it = range.first; it != range.second; ++it) {
            QRect const &i_r = it->rect().translated(-hscroll, 0);
            auto const text_pos =
                i_r.topLeft() +
                QPointF(
                    TagsRectsCalculator::tag_inner_left_padding,
                    ifce->fontMetrics().ascent() +
                        ((i_r.height() - ifce->fontMetrics().height()) / 2.0f));

            // drag tag rect
            QColor const blue(225, 236, 244);
            QPainterPath path;
            path.addRoundedRect(i_r, 4, 4);
            p.fillPath(path, blue);

            // draw text
            p.drawText(text_pos, it->text());

            // calc cross rect
            auto const i_cross_r =
                TagsRectsCalculator::localCloseTagCrossRect(i_r);

            QPen pen = p.pen();
            pen.setWidth(2);

            p.save();
            p.setPen(pen);
            p.setRenderHint(QPainter::Antialiasing);
            p.drawLine(QLineF(i_cross_r.topLeft(), i_cross_r.bottomRight()));
            p.drawLine(QLineF(i_cross_r.bottomLeft(), i_cross_r.topRight()));
            p.restore();
        }
    }

    void setCursorVisible(bool visible)
    {
        if (blink_timer) {
            ifce->killTimer(blink_timer);
            blink_timer = 0;
            blink_status = true;
        }

        if (visible) {
            const int flashTime =
                QGuiApplication::styleHints()->cursorFlashTime();
            if (flashTime >= 2) {
                blink_timer = ifce->startTimer(flashTime / 2);
            }
        } else {
            blink_status = false;
        }
    }

    [[nodiscard]]
    bool cursorVisible() const
    {
        return blink_timer;
    }

    void updateCursorBlinking() { setCursorVisible(cursorVisible()); }

    void updateDisplayText()
    {
        text_layout.clearLayout();
        text_layout.setText(currentTag().text());
        text_layout.beginLayout();
        text_layout.createLine();
        text_layout.endLayout();
    }

    void setEditingIndex(size_t i)
    {
        assert(i <= tags.size());
        if (currentTag().isEmpty()) {
            tags.erase(std::next(tags.begin(),
                                 static_cast<std::ptrdiff_t>(editing_index)));
            if (editing_index <= i) {
                --i;
            }
        }
        editing_index = i;
    }

    void calcRects()
    {
        const TagsRectsCalculator calc(*ifce);
        const auto index = editing_index;
        if (index < 0 || index >= tags.size()) {
            calc.updateRects(tags.begin(), tags.end());
            return;
        }
        calc.updateRects(
            tags.begin(), tags.end(),
            EditingTagData{ std::next(tags.begin(), index), &text_layout });
    }

    void currentTag(QString const &text)
    {
        currentTag().setText(text);
        moveCursor(text.length(), false);
        updateDisplayText();
        calcRects();
        ifce->update();
    }

    [[nodiscard]]
    VisualTag const &currentTag() const
    {
        return tags[editing_index];
    }

    [[nodiscard]]
    VisualTag &currentTag()
    {
        return tags[editing_index];
    }

    void addAndEditNewTag()
    {
        tags.emplace_back();
        setEditingIndex(tags.size() - 1);
        moveCursor(0, false);
    }

    void setupCompleter()
    {
        completer->setWidget(ifce);
        connect(completer.get(),
                qOverload<QString const &>(&QCompleter::activated),
                [this](QString const &text) { currentTag(text); });
    }

    QVector<QTextLayout::FormatRange> formatting() const
    {
        if (select_size == 0) {
            return {};
        }

        QTextLayout::FormatRange selection;
        selection.start = select_start;
        selection.length = select_size;
        selection.format.setBackground(
            ifce->palette().brush(QPalette::Highlight));
        selection.format.setForeground(
            ifce->palette().brush(QPalette::HighlightedText));
        return { selection };
    }

    [[nodiscard]]
    bool hasSelection() const noexcept
    {
        return select_size > 0;
    }

    void removeSelection()
    {
        cursor = select_start;
        currentTag().remove(cursor, select_size);
        deselectAll();
    }

    void removeBackwardOne()
    {
        if (hasSelection()) {
            removeSelection();
        } else {
            currentTag().remove(--cursor, 1);
        }
    }

    void removePreviousTag()
    {
        if (hasSelection()) {
            removeSelection();
            return;
        }
        if ((currentTag().size() == 0) && (editing_index > 0)) {
            setEditingIndex(editing_index - 1);
            moveCursor(currentTag().size(), false);
        }
        cursor -= currentTag().size();
        currentTag().remove(cursor, currentTag().size());
    }

    void selectAll()
    {
        select_start = 0;
        select_size = currentTag().size();
    }

    void deselectAll()
    {
        select_start = 0;
        select_size = 0;
    }

    void moveCursor(int pos, bool mark)
    {
        if (mark) {
            const auto e = select_start + select_size;
            const int anchor = select_size > 0 && cursor == select_start ? e
                               : select_size > 0 && cursor == e ? select_start
                                                                : cursor;
            select_start = qMin(anchor, pos);
            select_size = qMax(anchor, pos) - select_start;
        } else {
            deselectAll();
        }

        cursor = pos;
    }

    [[nodiscard]]
    qreal natrualWidth() const
    {
        return tags.back().rect().right() - tags.front().rect().left();
    }

    [[nodiscard]]
    qreal cursorToX() const
    {
        return text_layout.lineAt(0).cursorToX(cursor);
    }

    void calcHScroll(QRect const &r)
    {
        auto const rect = TagsRectsCalculator(*ifce).localContentRect();
        auto const width_used = qRound(natrualWidth()) + 1;
        int const cix = r.x() + qRound(cursorToX());
        if (width_used <= rect.width()) {
            // text fit
            hscroll = 0;
        } else if (cix - hscroll >= rect.width()) {
            // text doesn't fit, cursor is to the right of lineRect (scroll
            // right)
            hscroll = cix - rect.width() + 1;
        } else if (cix - hscroll < 0 && hscroll < width_used) {
            // text doesn't fit, cursor is to the left of lineRect (scroll left)
            hscroll = cix;
        } else if (width_used - hscroll < rect.width()) {
            // text doesn't fit, text document is to the left of lineRect; align
            // right
            hscroll = width_used - rect.width() + 1;
        } else {
            // in case the text is bigger than the lineedit, the hscroll can
            // never be negative
            hscroll = qMax(0, hscroll);
        }
    }

    void editPreviousTag()
    {
        if (editing_index > 0) {
            setEditingIndex(editing_index - 1);
            moveCursor(currentTag().size(), false);
        }
    }

    void editNextTag()
    {
        if (editing_index < tags.size() - 1) {
            setEditingIndex(editing_index + 1);
            moveCursor(0, false);
        }
    }

    void editTag(size_t i)
    {
        // assert(i >= 0 && i < tags.size());
        setEditingIndex(i);
        moveCursor(currentTag().size(), false);
    }

    TagsEdit *const ifce;
    std::vector<VisualTag> tags{ VisualTag() };
    size_t editing_index{ 0 };
    int cursor{ 0 };
    int blink_timer{ 0 };
    bool blink_status{ true };
    QTextLayout text_layout;
    int select_start{ 0 };
    int select_size{ 0 };
    QInputControl ctrl{ QInputControl::LineEdit };
    std::unique_ptr<QCompleter> completer{ std::make_unique<QCompleter>() };
    int hscroll{ 0 };
};

TagsEdit::TagsEdit(QWidget *parent)
    : QWidget(parent)
    , impl(std::make_unique<Impl>(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::IBeamCursor);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);

    impl->setupCompleter();
    impl->setCursorVisible(hasFocus());
    impl->updateDisplayText();
}

TagsEdit::~TagsEdit() = default;

void TagsEdit::resizeEvent(QResizeEvent *) { impl->calcRects(); }

void TagsEdit::focusInEvent(QFocusEvent *)
{
    impl->setCursorVisible(true);
    impl->updateDisplayText();
    impl->calcRects();
    update();
}

void TagsEdit::focusOutEvent(QFocusEvent *)
{
    impl->setCursorVisible(false);
    impl->updateDisplayText();
    impl->calcRects();
    update();
}

void TagsEdit::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const TagsRectsCalculator calc(*this);
    // opt
    auto const panel = [this, &calc] {
        QStyleOptionFrame panel;
        calc.initStyleOption(panel);
        return panel;
    }();

    // draw frame
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);

    // clip
    auto const rect = calc.localContentRect();
    p.setClipRect(rect);

    if (impl->cursorVisible()) {
        // not terminated tag pos
        auto const &r = impl->currentTag().rect();
        auto const &txt_p =
            r.topLeft() +
            QPointF(TagsRectsCalculator::tag_inner_left_padding,
                    ((r.height() - fontMetrics().height()) / 2.0f));

        // scroll
        impl->calcHScroll(r);

        // tags
        impl->drawTags(p, std::make_pair(impl->tags.cbegin(),
                                         std::next(impl->tags.cbegin(),
                                                   static_cast<std::ptrdiff_t>(
                                                       impl->editing_index))));

        // draw not terminated tag
        auto const formatting = impl->formatting();
        impl->text_layout.draw(&p, txt_p - QPointF(impl->hscroll, 0),
                               formatting);

        // draw cursor
        if (impl->blink_status) {
            impl->text_layout.drawCursor(&p, txt_p - QPointF(impl->hscroll, 0),
                                         impl->cursor);
        }

        // tags
        impl->drawTags(p,
                       std::make_pair(std::next(impl->tags.cbegin(),
                                                static_cast<std::ptrdiff_t>(
                                                    impl->editing_index + 1)),
                                      impl->tags.cend()));
    } else {
        const PredicateSkipIterator iter(impl->tags.begin(), impl->tags.end(),
                                         TagsRectsCalculator::m_filter);
        impl->drawTags(p, std::make_pair(iter, iter.make_end()));
    }
}

void TagsEdit::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == impl->blink_timer) {
        impl->blink_status = !impl->blink_status;
        update();
    }
}

void TagsEdit::mousePressEvent(QMouseEvent *event)
{
    bool foundClickedTag = false;
    for (size_t i = 0; i < impl->tags.size(); ++i) {
        if (impl->inCrossArea(i, event->pos())) {
            impl->tags.erase(impl->tags.begin() +
                             static_cast<std::ptrdiff_t>(i));
            if (i <= impl->editing_index) {
                --impl->editing_index;
            }
            emit tagsChanged();
            foundClickedTag = true;
            break;
        }

        if (!impl->tags[i]
                 .rect()
                 .translated(-impl->hscroll, 0)
                 .contains(event->pos())) {
            continue;
        }

        if (impl->editing_index == i) {
            impl->moveCursor(
                impl->text_layout.lineAt(0).xToCursor(
                    (event->pos() - impl->currentTag()
                                        .rect()
                                        .translated(-impl->hscroll, 0)
                                        .topLeft())
                        .x()),
                false);
        } else {
            impl->editTag(i);
        }

        foundClickedTag = true;
        break;
    }

    if (!foundClickedTag) {
        impl->addAndEditNewTag();
        event->accept();
    }

    if (event->isAccepted()) {
        impl->updateDisplayText();
        impl->calcRects();
        impl->updateCursorBlinking();
        update();
        emit tagsChanged();
    }
}

QSize TagsEdit::sizeHint() const
{
    ensurePolished();
    const QFontMetrics fm(font());
    const int h = fm.height() + 2 * vertical_margin +
                  TagsRectsCalculator::top_text_margin +
                  TagsRectsCalculator::bottom_text_margin + topmargin +
                  bottommargin;
    const int w = fm.boundingRect(QLatin1Char('x')).width() * 17 +
                  2 * horizontal_margin + leftmargin + rightmargin; // "some"
    QStyleOptionFrame opt;
    TagsRectsCalculator(*this).initStyleOption(opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt,
                                      QSize(w, h).expandedTo(globalStrut()),
                                      this));
}

QSize TagsEdit::minimumSizeHint() const
{
    ensurePolished();
    const QFontMetrics fm = fontMetrics();
    const int h = fm.height() + qMax(2 * vertical_margin, fm.leading()) +
                  TagsRectsCalculator::top_text_margin +
                  TagsRectsCalculator::bottom_text_margin + topmargin +
                  bottommargin;
    const int w = fm.maxWidth() + leftmargin + rightmargin;
    QStyleOptionFrame opt;
    TagsRectsCalculator(*this).initStyleOption(opt);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt,
                                      QSize(w, h).expandedTo(globalStrut()),
                                      this));
}

void TagsEdit::keyPressEvent(QKeyEvent *event)
{
    event->setAccepted(false);
    bool eventWasNotProcessed = false;
    if (event == QKeySequence::SelectAll) {
        impl->selectAll();
        event->accept();
    } else if (event == QKeySequence::SelectPreviousChar) {
        impl->moveCursor(impl->text_layout.previousCursorPosition(impl->cursor),
                         true);
        event->accept();
    } else if (event == QKeySequence::SelectNextChar) {
        impl->moveCursor(impl->text_layout.nextCursorPosition(impl->cursor),
                         true);
        event->accept();
    } else if (event == QKeySequence::DeleteStartOfWord) {
        impl->removePreviousTag();
        emit tagsChanged();
        event->accept();
    } else {
        switch (event->key()) {
        case Qt::Key_Left:
            if (impl->cursor == 0) {
                impl->editPreviousTag();
            } else {
                impl->moveCursor(
                    impl->text_layout.previousCursorPosition(impl->cursor),
                    false);
            }
            event->accept();
            break;
        case Qt::Key_Right:
            if (impl->cursor == impl->currentTag().size()) {
                impl->editNextTag();
            } else {
                impl->moveCursor(
                    impl->text_layout.nextCursorPosition(impl->cursor), false);
            }
            event->accept();
            break;
        case Qt::Key_Home:
            if (impl->cursor == 0) {
                impl->editTag(0);
            } else {
                impl->moveCursor(0, false);
            }
            event->accept();
            break;
        case Qt::Key_End:
            if (impl->cursor == impl->currentTag().size()) {
                impl->editTag(impl->tags.size() - 1);
            } else {
                impl->moveCursor(impl->currentTag().size(), false);
            }
            event->accept();
            break;
        case Qt::Key_Backspace:
            if (!impl->currentTag().isEmpty()) {
                impl->removeBackwardOne();
            } else if (impl->editing_index > 0) {
                impl->editPreviousTag();
            }
            event->accept();
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Space:
            if (!impl->currentTag().isEmpty()) {
                impl->tags.insert(
                    impl->tags.begin() +
                        static_cast<std::ptrdiff_t>(impl->editing_index + 1),
                    VisualTag());
                impl->editNextTag();
                emit tagsChanged();
            }
            event->accept();
            break;
        default:
            eventWasNotProcessed = true;
            break;
        }
    }

    if (eventWasNotProcessed && impl->ctrl.isAcceptableInput(event)) {
        if (impl->hasSelection()) {
            impl->removeSelection();
        }
        impl->currentTag().insert(impl->cursor, event->text());
        impl->cursor += event->text().length();
        event->accept();
    }

    if (event->isAccepted()) {
        // update content
        impl->updateDisplayText();
        impl->calcRects();
        impl->updateCursorBlinking();

        // complete
        impl->completer->setCompletionPrefix(impl->currentTag().text());
        impl->completer->complete();

        update();
    }
}

void TagsEdit::completion(std::vector<QString> const &completions)
{
    impl->completer = std::make_unique<QCompleter>([&] {
        QStringList ret;
        std::copy(completions.begin(), completions.end(),
                  std::back_inserter(ret));
        return ret;
    }());
    impl->setupCompleter();
}

void TagsEdit::setTags(const QStringList &tags)
{
    std::vector<VisualTag> t;
    t.reserve(tags.size() + 1u);
    t.emplace_back(); // Adding empty tag
    std::transform(tags.begin(), tags.end(), std::back_inserter(t),
                   [](const auto &tt) { return VisualTag{ tt, QRect() }; });
    impl->tags = std::move(t);
    impl->editing_index = 0;
    impl->moveCursor(0, false);

    impl->addAndEditNewTag();
    impl->updateDisplayText();
    impl->calcRects();

    update();
}

QStringList TagsEdit::getTags() const
{
    QStringList res;
    const PredicateSkipIterator iter(impl->tags.begin(), impl->tags.end(),
                                     TagsRectsCalculator::m_filter);
    std::transform(iter, iter.make_end(), std::back_inserter(res),
                   [](const VisualTag &tag) { return tag.text(); });
    return res;
}

void TagsEdit::clearTags() { setTags(QStringList{}); }

void TagsEdit::pushTag(const QString &value)
{
    for (const auto &t : impl->tags) {
        if (value == t.text()) {
            return;
        }
    }

    auto t = VisualTag();
    t.setText(value);
    impl->tags.push_back(t);
    impl->updateDisplayText();
    impl->calcRects();
    update();
    emit tagsChanged();
}

void TagsEdit::popTag()
{
    if (impl->tags.empty()) {
        return;
    }
    impl->tags.pop_back();
    update();
}

void TagsEdit::mouseMoveEvent(QMouseEvent *event)
{
    for (size_t i = 0; i < impl->tags.size(); ++i) {
        if (impl->inCrossArea(i, event->pos())) {
            setCursor(Qt::ArrowCursor);
            return;
        }
    }
    setCursor(Qt::IBeamCursor);
}
