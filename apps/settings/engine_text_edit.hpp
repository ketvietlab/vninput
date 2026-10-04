#pragma once

#include <vninput/engine.hpp>
#include <QTextEdit>

namespace vninput::ui {

// Preview adapter only. Native system input contexts use the same core contract.
class EngineTextEdit final : public QTextEdit {
    Q_OBJECT
public:
    explicit EngineTextEdit(QWidget* parent = nullptr);
    void configureEngine(const Settings& settings);
    QString visibleText() const;
    QString preeditText() const { return preedit_; }

signals:
    void visibleTextChanged();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void inputMethodEvent(QInputMethodEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void insertFromMimeData(const QMimeData* source) override;
    void dropEvent(QDropEvent* event) override;

private:
    void apply(const Composition& result);
    void commitPending();
    std::unique_ptr<Engine> engine_ = makeEngine();
    Settings settings_;
    QString preedit_;
    QString osPreedit_;
    int preeditPosition_ = 0;
    bool applying_ = false;
    bool externalPreedit_ = false;
};

} // namespace vninput::ui
