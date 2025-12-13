#include "tagsedit.hpp"

#include <QDebug>

#include <QApplication>
#include <QChar>
#include <QClipboard>
#include <QColor>
#include <QCompleter>
#include <QDebug>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMouseEvent>
#include <QOverload>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QSize>
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
#include <list>
#include <memory>
#include <set>
#include <stdexcept>
#include <vector>

#include "tags_drawer.hpp"
#include "tags_rects_calculator.h"
#include "visual_tag.hpp"

namespace
{

constexpr int kTagsChangedDebounceMs = 750;

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

// We need container which will keep iterators valid when elements were added or
// removed. Note, if we use .clear() or std::move(), than .end() iterator will
// become invalid. This should be handled.
using TagsCollection = std::list<VisualTag>;
enum class NextPrevTagBehave : std::uint8_t {
    KeepCurrent,
    EraseCurrentIfEmpty
};
} // namespace

/// @brief Helper to detect when tags were actually changed.
class TagsEdit::TagChangedDetector {
  public:
    /// @returns true if it was change since last call to update() (or it is 1st
    /// one).
    [[nodiscard]]
    bool update(TagsCollection &latest_tags)
    {
        std::size_t sz = 0u;
        bool hadModification = false;

        TagsDrawer::ForEachDrawnTagIter(latest_tags, [&](auto iter) {
            ++sz;
            hadModification = hadModification || iter->isModified();
            iter->setNotModified();
            return true;
        });

        const bool sizeChanged = sz != last_known_size;
        last_known_size = sz;
        return sizeChanged || hadModification;
    }

  private:
    std::size_t last_known_size{ 0u };
    bool had_modification_of_non_empty{ false };
};

class TagsEdit::Impl : public IDrawerState {
  public:
    explicit Impl(TagsEdit *const &ifce)
        : IDrawerState()
        , ifce(ifce)
    {
        endEditing();
    }

    ~Impl() override = default;

    [[nodiscard]]
    bool isPointOnCloseButton(TagsCollection::iterator iter,
                              QPoint const &point) const
    {
        return (!cursorVisible() || iter != editing_iter) &&
               iter != tags.end() &&
               TagsDrawer::isPointOnCloseButton(*this, iter, point);
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
        if (isEditing()) {
            text_layout.setText(currentEditedTag().text());
            text_layout.beginLayout();
            text_layout.createLine();
            text_layout.endLayout();
        }
    }

    void setEditingIndex(TagsCollection::iterator iter) { editing_iter = iter; }

    [[nodiscard]]
    bool isEditing() const
    {
        return editing_iter != tags.end();
    }

    void calcRects()
    {
        const TagsRectsCalculator calc(*ifce);
        if (isEditing()) {
            calc.updateRects(tags.begin(), tags.end(),
                             EditingTagData{ editing_iter, &text_layout });
            return;
        }
        calc.updateRects(tags.begin(), tags.end());
    }

    void setCurrentEditedTag(QString const &text)
    {
        if (!isEditing()) {
            return;
        }
        currentEditedTag().setText(text);
        moveCursor(text.length(), false);
        updateDisplayText();
        calcRects();
        ifce->update();
    }

    [[nodiscard]]
    VisualTag const &currentEditedTag() const
    {
        if (!isEditing()) {
            throw std::runtime_error("Access to editable tag without editing.");
        }
        return *editing_iter;
    }

    [[nodiscard]]
    VisualTag &currentEditedTag()
    {
        if (!isEditing()) {
            throw std::runtime_error("Access to editable tag without editing.");
        }
        return *editing_iter;
    }

    void addAndEditNewTag()
    {
        tags.emplace_back();
        setEditingIndex(std::prev(tags.end()));
        moveCursor(0, false);
    }

    void setupCompleter()
    {
        completer->setWidget(ifce);
        connect(completer.get(),
                qOverload<QString const &>(&QCompleter::activated),
                [this](QString const &text) { setCurrentEditedTag(text); });
    }

    [[nodiscard]]
    bool hasSelection() const noexcept
    {
        return select_size > 0;
    }

    void removeSelection()
    {
        m_cursor = select_start;
        currentEditedTag().remove(m_cursor, select_size);
        deselectAll();
    }

    // Backspace button.
    void removeBackwardOne()
    {
        if (hasSelection()) {
            removeSelection();
        } else {
            currentEditedTag().remove(--m_cursor, 1);
        }
    }

    // DEL button.
    void removeFrontOne()
    {
        if (hasSelection()) {
            removeSelection();
        } else {
            currentEditedTag().remove(m_cursor, 1);
        }
    }

    void endEditing() { editing_iter = tags.end(); }

    void eraseCurrentEditedTag()
    {
        if (isEditing()) {
            tags.erase(editing_iter);
            endEditing();
        }
    }

    [[nodiscard]]
    bool isEditedEmpty() const
    {
        if (!isEditing()) {
            return false;
        }
        return TagsRectsCalculator::m_filter(currentEditedTag());
    }

    void removePreviousTag()
    {
        if (!isEditing()) {
            return;
        }

        if (hasSelection()) {
            removeSelection();
            return;
        }

        if (isEditedEmpty()) {
            // Tag which should not be considered (empty).
            if (editing_iter != tags.begin()) {
                // Tag was NOT 1st.
                auto prev = std::prev(editing_iter);
                eraseCurrentEditedTag();
                setEditingIndex(prev);
                moveCursor(prev->size(), false);
                return;
            }
            // It was 1st empty tag, just erase and set cursor to 0
            eraseCurrentEditedTag();
            setEditingIndex(tags.begin());
            moveCursor(0, false);
            if (tags.begin() != tags.end()) {
                moveCursor(tags.begin()->size(), false);
            }
            return;
        }

        // Deleting part of the tag before cursor
        if (m_cursor > 0) {
            currentEditedTag().remove(0, m_cursor);
            moveCursor(0, false);
            if (isEditedEmpty()) {
                const auto offset = std::distance(tags.begin(), editing_iter);
                eraseCurrentEditedTag();
                if (offset < tags.size()) {
                    editTag(std::next(tags.begin(), offset));
                } else {
                    addOrEditLast();
                }
            }
        }
    }

    void selectAll()
    {
        select_start = 0;
        select_size = currentEditedTag().size();
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
            const int anchor = select_size > 0 && m_cursor == select_start ? e
                               : select_size > 0 && m_cursor == e ? select_start
                                                                  : m_cursor;
            select_start = qMin(anchor, pos);
            select_size = qMax(anchor, pos) - select_start;
        } else {
            deselectAll();
        }

        m_cursor = pos;
    }

    void editPreviousTag(const NextPrevTagBehave behave)
    {
        if (!isEditing()) {
            return;
        }
        if (editing_iter != tags.begin()) {
            auto prev = std::prev(editing_iter);
            if (behave == NextPrevTagBehave::EraseCurrentIfEmpty) {
                if (TagsRectsCalculator::m_filter(currentEditedTag())) {
                    eraseCurrentEditedTag();
                }
            }
            setEditingIndex(prev);
            moveCursor(currentEditedTag().size(), false);
        }
    }

    void editNextTag(const NextPrevTagBehave behave)
    {
        if (!isEditing()) {
            return;
        }
        auto nxt = std::next(editing_iter, 1);
        if (nxt != tags.end()) {
            if (behave == NextPrevTagBehave::EraseCurrentIfEmpty) {
                if (TagsRectsCalculator::m_filter(currentEditedTag())) {
                    eraseCurrentEditedTag();
                }
            }
            setEditingIndex(nxt);
            moveCursor(0, false);
        }
    }

    void editTag(TagsCollection::iterator iter)
    {
        setEditingIndex(iter);
        moveCursor(currentEditedTag().size(), false);
    }

    void addOrEditLast()
    {
        if (tags.empty()) {
            addAndEditNewTag();
            return;
        }
        editTag(std::prev(tags.end(), 1));
    }

    void setTags(const QStringList &tags)
    {
        this->tags.clear();
        std::transform(tags.begin(), tags.end(), std::back_inserter(this->tags),
                       [](const auto &tt) { return VisualTag{ tt, QRect() }; });
        endEditing();
    }

    void addSingleStringTag(const QString &str)
    {
        auto tag = str.trimmed();
        if (tag.contains(' ')) {
            assert(false &&
                   "Adding many tags in single string is not implemented.");
            return;
        }
        if (tags.end() !=
            std::find_if(tags.begin(), tags.end(), [&tag](const auto &vis) {
                return tag == vis.text();
            })) {
            return;
        }
        endEditing();
        sanitizeTags();
        tags.emplace_back(VisualTag("", {}));
        tags.back().setText(tag); // triggerring "modified" flag.
    }

    static void sanitizeTags(TagsCollection &tags)
    {
        std::set<QString> seen;

        for (auto it = tags.begin(); it != tags.end();) {
            const bool duplicate = seen.find(it->text()) != seen.end();
            if (TagsRectsCalculator::m_filter(*it) || duplicate) {
                it = tags.erase(it);
            } else {
                seen.insert(it->text());
                ++it;
            }
        }
    }

    void sanitizeTags()
    {
        endEditing();
        sanitizeTags(tags);
    }

    TagsEdit *const ifce;
    TagsCollection tags{};
    TagsCollection::iterator editing_iter{ tags.end() };
    int m_cursor{ 0 };
    int blink_timer{ 0 };
    bool blink_status{ true };
    QTextLayout text_layout;
    int select_start{ 0 };
    int select_size{ 0 };
    QInputControl ctrl{ QInputControl::LineEdit };
    std::unique_ptr<QCompleter> completer{ std::make_unique<QCompleter>() };
    int hscroll{ 0 };

    // IDrawerState interface
  public:
    [[nodiscard]]
    int hScroll() const override
    {
        return hscroll;
    }
    void setHScroll(int value) override { hscroll = value; }

    [[nodiscard]]
    int cursor() const override
    {
        return m_cursor;
    }
    [[nodiscard]]
    int isDrawCursor() const override
    {
        return blink_status;
    }
    [[nodiscard]]
    Selection selection() const override
    {
        return { select_start, select_size };
    }
};

TagsEdit::TagsEdit(QWidget *parent)
    : QWidget(parent)
    , impl(std::make_unique<Impl>(this))
    , change_detector(std::make_unique<TagChangedDetector>())
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::IBeamCursor);
    setAttribute(Qt::WA_InputMethodEnabled, true);
    setMouseTracking(true);

    impl->setupCompleter();
    impl->setCursorVisible(hasFocus());
    impl->updateDisplayText();

    debouncedTagsChanged.setSingleShot(true);
    debouncedTagsChanged.setInterval(kTagsChangedDebounceMs);

    connect(&debouncedTagsChanged, &QTimer::timeout, this,
            [this]() { emit tagsChanged(); });
}

TagsEdit::~TagsEdit() = default;

void TagsEdit::resizeEvent(QResizeEvent *) { impl->calcRects(); }

void TagsEdit::focusInEvent(QFocusEvent *)
{
    impl->setCursorVisible(true);
    updateAndCheckForChanges();
}

void TagsEdit::focusOutEvent(QFocusEvent *)
{
    impl->sanitizeTags();
    impl->setCursorVisible(false);
    updateAndCheckForChanges();
}

void TagsEdit::paintEvent(QPaintEvent *)
{
    const TagsDrawer drawer(*this);
    if (impl->isEditing()) {
        drawer.drawTags(
            *impl, impl->tags.begin(), impl->tags.end(),
            EditingTagData{ impl->editing_iter, &impl->text_layout });
        return;
    }
    drawer.drawTags(*impl, impl->tags.begin(), impl->tags.end());
}

void TagsEdit::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == impl->blink_timer) {
        impl->blink_status = !impl->blink_status;
        update();
    }
}

void TagsEdit::mouseMoveEvent(QMouseEvent *event)
{
    setCursor(Qt::IBeamCursor);
    TagsDrawer::ForEachDrawnTagIter(
        impl->tags, [&event, this](const auto iter) {
            if (impl->isPointOnCloseButton(iter, event->pos())) {
                setCursor(Qt::ArrowCursor);
                return false;
            }
            return true;
        });
}
void TagsEdit::mousePressEvent(QMouseEvent *event)
{
    event->accept();

    const bool newTagNeeded = TagsDrawer::ForEachDrawnTagIter(
        impl->tags, [&event, this](const auto iter) {
            if (impl->isPointOnCloseButton(iter, event->pos())) {
                // Clicked "close button".
                impl->setEditingIndex(iter);
                impl->eraseCurrentEditedTag();
                return false;
            }
            if (!iter->rect()
                     .translated(-impl->hscroll, 0)
                     .contains(event->pos())) {
                return true;
            }

            // Found clicked area.
            if (impl->editing_iter == iter) {
                // It was already edited, reposition the cursor.
                impl->moveCursor(
                    impl->text_layout.lineAt(0).xToCursor(
                        (event->pos() - impl->currentEditedTag()
                                            .rect()
                                            .translated(-impl->hscroll, 0)
                                            .topLeft())
                            .x()),
                    false);
            } else {
                // Start editing.
                impl->editTag(iter);
            }
            return false;
        });

    if (newTagNeeded) {
        impl->addAndEditNewTag();
    }
    impl->updateCursorBlinking();
    updateAndCheckForChanges();
}

QSize TagsEdit::sizeHint() const
{
    ensurePolished();
    const QFontMetrics fm(font());
    const int h = fm.height() + 2 * vertical_margin +
                  TagsRectsCalculator::verticalTextMargins() + topmargin +
                  bottommargin;
    const int w = fm.boundingRect(QLatin1Char('x')).width() * 17 +
                  2 * horizontal_margin + leftmargin + rightmargin; // "some"
    auto opt = TagsRectsCalculator::createStyleOption(*this);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt,
                                      QSize(w, h).expandedTo(globalStrut()),
                                      this));
}

QSize TagsEdit::minimumSizeHint() const
{
    ensurePolished();
    const QFontMetrics fm = fontMetrics();
    const int h = fm.height() + qMax(2 * vertical_margin, fm.leading()) +
                  TagsRectsCalculator::verticalTextMargins() + topmargin +
                  bottommargin;
    const int w = fm.maxWidth() + leftmargin + rightmargin;
    auto opt = TagsRectsCalculator::createStyleOption(*this);
    return (style()->sizeFromContents(QStyle::CT_LineEdit, &opt,
                                      QSize(w, h).expandedTo(globalStrut()),
                                      this));
}

void TagsEdit::keyPressEvent(QKeyEvent *event)
{
    event->setAccepted(true);

    const auto pasteTag = [this]() {
        const QString text = QGuiApplication::clipboard()->text();
        if (!text.isEmpty()) {
            impl->addSingleStringTag(text);
        }
    };
    if (!impl->isEditing()) {
        switch (event->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Space:
            impl->addOrEditLast();
            break;
        default:
            return;
        }
    }

    bool eventWasNotProcessed = false;
    if (event == QKeySequence::SelectAll) {
        impl->selectAll();
    } else if (event == QKeySequence::SelectPreviousChar) {
        impl->moveCursor(
            impl->text_layout.previousCursorPosition(impl->m_cursor), true);
    } else if (event == QKeySequence::SelectNextChar) {
        impl->moveCursor(impl->text_layout.nextCursorPosition(impl->m_cursor),
                         true);
    } else if (event == QKeySequence::Paste) {
        if (impl->currentEditedTag().isEmpty()) {
            pasteTag();
        }
    } else if (event == QKeySequence::DeleteStartOfWord) {
        impl->removePreviousTag();
    } else {
        switch (event->key()) {
        case Qt::Key_Left:
            if (impl->m_cursor == 0) {
                impl->editPreviousTag(NextPrevTagBehave::KeepCurrent);
            } else {
                impl->moveCursor(
                    impl->text_layout.previousCursorPosition(impl->m_cursor),
                    false);
            }
            break;
        case Qt::Key_Right:
            if (impl->m_cursor == impl->currentEditedTag().size()) {
                impl->editNextTag(NextPrevTagBehave::KeepCurrent);
            } else {
                impl->moveCursor(
                    impl->text_layout.nextCursorPosition(impl->m_cursor),
                    false);
            }
            break;
        case Qt::Key_Home:
            if (impl->m_cursor == 0) {
                impl->editTag(impl->tags.begin());
            } else {
                impl->moveCursor(0, false);
            }
            break;
        case Qt::Key_End:
            if (impl->m_cursor == impl->currentEditedTag().size()) {
                impl->editTag(std::prev(impl->tags.end(), 1));
            } else {
                impl->moveCursor(impl->currentEditedTag().size(), false);
            }
            break;
        case Qt::Key_Backspace:
            if (!impl->currentEditedTag().isEmpty() &&
                (impl->cursor() > 0 || impl->hasSelection())) {
                impl->removeBackwardOne();
            } else if (impl->editing_iter != impl->tags.begin()) {
                impl->editPreviousTag(NextPrevTagBehave::EraseCurrentIfEmpty);
            }
            break;
        case Qt::Key_Delete:
            if (!impl->currentEditedTag().isEmpty()) {
                impl->removeFrontOne();
            } else {
                impl->editNextTag(NextPrevTagBehave::EraseCurrentIfEmpty);
            }
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Space:
            if (!impl->currentEditedTag().isEmpty()) {
                impl->addAndEditNewTag();
            } else {
                impl->endEditing();
            }
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
        impl->currentEditedTag().insert(impl->m_cursor, event->text());
        impl->m_cursor += event->text().length();
    }

    // update content
    impl->updateCursorBlinking();
    // complete
    if (impl->isEditing()) {
        impl->completer->setCompletionPrefix(impl->currentEditedTag().text());
        impl->completer->complete();
    }

    updateAndCheckForChanges();
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
    impl->setTags(tags);
    impl->moveCursor(0, false);
    updateAndCheckForChanges();
}

QStringList TagsEdit::getTags() const
{
    auto tmp = impl->tags;
    Impl::sanitizeTags(tmp);

    QStringList res;
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(res),
                   [](const VisualTag &tag) { return tag.text(); });
    return res;
}

void TagsEdit::clearTags() { setTags(QStringList{}); }

void TagsEdit::pushTag(const QString &value)
{
    impl->addSingleStringTag(value);
    updateAndCheckForChanges();
}

void TagsEdit::updateAndCheckForChanges()
{
    impl->updateDisplayText();
    impl->calcRects();
    update();

    if (change_detector->update(impl->tags)) {
        debouncedTagsChanged.start();
    }
}
