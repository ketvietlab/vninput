#include "settings_window.hpp"
#include "theme.hpp"
#include "engine_text_edit.hpp"
#include <QApplication>
#include <QDir>
#include <QListWidget>
#include <QTemporaryDir>
#include <QFontInfo>
#include <QDebug>
#include <QPushButton>
#include <QKeyEvent>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    if (argc != 2 && !(argc == 3 && QString::fromLocal8Bit(argv[2]) == "--engine")) return 2;
    const QDir output(QString::fromLocal8Bit(argv[1]));
    if (!QDir().mkpath(output.absolutePath())) return 3;
    vninput::ui::applyTheme(app);
    qInfo() << "UI font:" << QFontInfo(app.font()).family() << QFontInfo(app.font()).styleName();
    QTemporaryDir temporary;
    vninput::ui::SettingsWindow window(temporary.filePath("settings.json"));
    window.show();
    if (argc == 3) {
        window.findChild<QListWidget*>("navigation")->setCurrentRow(2);
        auto* editor = window.findChild<vninput::ui::EngineTextEdit*>("typing");
        editor->setFocus();
        const QString input = "Tooi yeeu tieengs Vieetj.\nDDuowngf veef quee thaatj ddepj.\nNguyeenx";
        for (const auto c : input) {
            const auto key = c == '\n' ? Qt::Key_Return : (c == ' ' ? Qt::Key_Space : c.toUpper().unicode());
            QKeyEvent event(QEvent::KeyPress, key, Qt::NoModifier, QString(c));
            QApplication::sendEvent(editor, &event);
        }
        if (editor->visibleText() != QStringLiteral("Tôi yêu tiếng Việt.\nĐường về quê thật đẹp.\nNguyễn")) return 5;
        QApplication::processEvents();
        return window.grab().save(output.filePath("engine-playground.png")) ? 0 : 4;
    }
    const QStringList pages{"settings", "shortcuts", "playground", "integration", "about"};
    for (int i = 0; i < pages.size(); ++i) {
        window.findChild<QListWidget*>("navigation")->setCurrentRow(i);
        QApplication::processEvents();
        window.clearFocus();
        if (!window.grab().save(output.filePath(pages[i] + ".png"))) return 4;
    }
    window.findChild<QListWidget*>("navigation")->setCurrentRow(0);
    window.findChild<QPushButton*>("defaults")->click();
    QApplication::processEvents();
    if (!window.grab().save(output.filePath("restore.png"))) return 4;
    window.findChild<QPushButton*>("defaults")->click();
    window.findChild<QPushButton*>("quickMenu")->click();
    QApplication::processEvents();
    for (auto* widget : window.findChildren<QWidget*>()) {
        if (widget->property("quickMenu").toBool()) {
            if (!widget->grab().save(output.filePath("quick-menu.png"))) return 4;
            widget->close(); break;
        }
    }
    return 0;
}
