#include "engine_text_edit.hpp"

#include <QContextMenuEvent>
#include <QDropEvent>
#include <QFocusEvent>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopedValueRollback>
#include <QTextCharFormat>

namespace vninput::ui {
namespace {
QString qtext(const std::u32string& text)
{
    return QString::fromUcs4(text.data(), static_cast<qsizetype>(text.size()));
}
}

EngineTextEdit::EngineTextEdit(QWidget* parent) : QTextEdit(parent)
{
    connect(document(), &QTextDocument::contentsChanged, this, [this] {
        if (applying_) return;
        // Undo, clear, setPlainText and external document edits invalidate the
        // old composition. Never replay it into a changed document.
        engine_->reset();
        preedit_.clear();
        osPreedit_.clear();
        externalPreedit_ = false;
        emit visibleTextChanged();
    });
    connect(this, &QTextEdit::cursorPositionChanged, this, [this] {
        if (applying_ || preedit_.isEmpty()) return;
        // Also handle setTextCursor() called by a host, outside key/mouse events.
        QScopedValueRollback guard(applying_, true);
        auto requestedCursor = textCursor();
        auto composingCursor = requestedCursor;
        composingCursor.setPosition(preeditPosition_);
        setTextCursor(composingCursor);
        commitPending();
        setTextCursor(requestedCursor);
    });
}

void EngineTextEdit::configureEngine(const Settings& settings)
{
    if (settings_ == settings) return;
    // Validate first, then commit with the old rules before applying new ones.
    engine_->configure(settings);
    commitPending();
    settings_ = settings;
}

QString EngineTextEdit::visibleText() const
{
    auto text = toPlainText();
    text.insert(qBound(0, preeditPosition_, static_cast<int>(text.size())), preedit_ + osPreedit_);
    return text;
}

void EngineTextEdit::apply(const Composition& result)
{
    QScopedValueRollback guard(applying_, true);
    preedit_ = qtext(result.preedit);
    QList<QInputMethodEvent::Attribute> attributes;
    if (!preedit_.isEmpty()) {
        QTextCharFormat format;
        format.setUnderlineStyle(QTextCharFormat::NoUnderline);
        attributes.append({QInputMethodEvent::TextFormat, 0, static_cast<int>(preedit_.size()), format});
        attributes.append({QInputMethodEvent::Cursor, static_cast<int>(preedit_.size()), 1, {}});
    }
    QInputMethodEvent event(preedit_, attributes);
    event.setCommitString(qtext(result.commit));
    QTextEdit::inputMethodEvent(&event);
    preeditPosition_ = textCursor().position();
    emit visibleTextChanged();
}

void EngineTextEdit::commitPending()
{
    const auto result = engine_->flush();
    if (!preedit_.isEmpty() || !result.commit.empty()) apply(result);
}

void EngineTextEdit::keyPressEvent(QKeyEvent* event)
{
    if (isReadOnly() || externalPreedit_) {
        commitPending();
        QTextEdit::keyPressEvent(event);
        return;
    }
    KeyEvent input;
    switch (event->key()) {
    case Qt::Key_Backspace: input.key = Key::Backspace; break;
    case Qt::Key_Escape: input.key = Key::Escape; break;
    case Qt::Key_Enter: case Qt::Key_Return: input.key = Key::Enter; break;
    case Qt::Key_Space: input.key = Key::Space; break;
    default: input.key = event->text().isEmpty() ? Key::Other : Key::Text; break;
    }
    // Modifier-only presses do not terminate a word, e.g. Shift for uppercase.
    if (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control
        || event->key() == Qt::Key_Alt || event->key() == Qt::Key_Meta) {
        QTextEdit::keyPressEvent(event);
        return;
    }
    for (const auto c : event->text().toUcs4()) input.text += static_cast<char32_t>(c);
    if (event->modifiers().testFlag(Qt::ShiftModifier)) input.modifiers |= static_cast<std::uint8_t>(Modifier::Shift);
    if (event->modifiers().testFlag(Qt::ControlModifier)) input.modifiers |= static_cast<std::uint8_t>(Modifier::Control);
    if (event->modifiers().testFlag(Qt::AltModifier)) input.modifiers |= static_cast<std::uint8_t>(Modifier::Alt);
    if (event->modifiers().testFlag(Qt::MetaModifier)) input.modifiers |= static_cast<std::uint8_t>(Modifier::Meta);
    const auto result = engine_->process(input);
    if (!preedit_.isEmpty() || !result.preedit.empty() || !result.commit.empty()) apply(result);
    if (result.consumed) event->accept();
    else QTextEdit::keyPressEvent(event);
}

void EngineTextEdit::inputMethodEvent(QInputMethodEvent* event)
{
    // Already composed text from the OS IME must never be run through Telex a
    // second time. This path is distinct from raw Latin key events above.
    commitPending();
    engine_->reset();
    QScopedValueRollback guard(applying_, true);
    externalPreedit_ = !event->preeditString().isEmpty();
    osPreedit_ = event->preeditString();
    QTextEdit::inputMethodEvent(event);
    preeditPosition_ = textCursor().position();
    emit visibleTextChanged();
}

void EngineTextEdit::focusOutEvent(QFocusEvent* event)
{
    commitPending();
    QTextEdit::focusOutEvent(event);
}
void EngineTextEdit::mousePressEvent(QMouseEvent* event)
{
    commitPending();
    QTextEdit::mousePressEvent(event);
}
void EngineTextEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    commitPending();
    QTextEdit::mouseDoubleClickEvent(event);
}
void EngineTextEdit::contextMenuEvent(QContextMenuEvent* event)
{
    commitPending();
    QTextEdit::contextMenuEvent(event);
}
void EngineTextEdit::insertFromMimeData(const QMimeData* source)
{
    commitPending();
    QTextEdit::insertFromMimeData(source);
}
void EngineTextEdit::dropEvent(QDropEvent* event)
{
    commitPending();
    QTextEdit::dropEvent(event);
}

} // namespace vninput::ui
