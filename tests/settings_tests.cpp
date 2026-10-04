#include "settings_store.hpp"
#include "settings_window.hpp"
#include "theme.hpp"
#include "engine_text_edit.hpp"
#ifdef VNINPUT_MACOS_IME
#include "native_settings.hpp"
#endif

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputMethodEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>

using namespace vninput;
using namespace vninput::ui;

class SettingsTests : public QObject {
    Q_OBJECT
private slots:
#ifdef VNINPUT_MACOS_IME
    void nativeConfigurationMatchesQt()
    {
        QCOMPARE(SettingsStore::defaultPath().toStdString(), macos::settingsPath());
        QTemporaryDir dir;
        SettingsStore store(dir.filePath("settings.json"));
        Settings settings;
        settings.inputMethod = InputMethod::Vni; settings.tonePlacement = TonePlacement::Classic;
        settings.enabled = false; settings.freeTone = false; settings.restoreOnEscape = false;
        settings.toggleShortcut = "Ctrl+Shift+V";
        QVERIFY(store.save(settings).isEmpty());
        const auto native = macos::readSettingsFile(store.path().toStdString());
        QVERIFY(native && *native == settings);
    }
#endif
    void roundTrip()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        SettingsStore store(dir.filePath("nested/settings.json"));
        QCOMPARE(store.load().status, LoadStatus::Missing);
        Settings settings;
        settings.inputMethod = InputMethod::Vni;
        settings.tonePlacement = TonePlacement::Classic;
        settings.enabled = false;
        settings.freeTone = false;
        settings.restoreOnEscape = false;
        settings.toggleShortcut = "Ctrl+Shift+V";
        QVERIFY(store.save(settings).isEmpty());
        QCOMPARE(store.load().status, LoadStatus::Loaded);
        QVERIFY(store.load().settings == settings);
        settings.toggleShortcut = "A";
        QVERIFY(!store.save(settings).isEmpty());
        QCOMPARE(store.load().settings.toggleShortcut, std::string("Ctrl+Shift+V"));
    }

    void rejectCorruptAndFutureConfig()
    {
        QTemporaryDir dir;
        SettingsStore store(dir.filePath("settings.json"));
        for (const auto& bytes : {QByteArray("not json"), QByteArray("[]"),
                 QByteArray("{\"schemaVersion\":999}"), QByteArray("{\"schemaVersion\":1}")}) {
            QFile file(store.path());
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(bytes), bytes.size());
            file.close();
            QCOMPARE(store.load().status, LoadStatus::Error);
            SettingsWindow window(store.path());
            QVERIFY(!window.findChild<QPushButton*>("save")->isEnabled());
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), bytes);
        }
    }

    void saveAndReloadThroughUi()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        SettingsWindow window(path);
        window.show();
        QApplication::processEvents();
        QCOMPARE(window.size(), QSize(640, 480));
        QVERIFY(!brandPixmap(25).isNull());
        auto* vni = window.findChild<QPushButton*>("vni");
        QTest::mouseClick(vni, Qt::LeftButton);
        QVERIFY(vni->isChecked());
        QTest::mouseClick(window.findChild<QPushButton*>("save"), Qt::LeftButton);
        QVERIFY(SettingsStore(path).load().settings.inputMethod == InputMethod::Vni);
        QVERIFY(window.close());
        SettingsWindow reopened(path);
        QVERIFY(reopened.findChild<QPushButton*>("vni")->isChecked());
    }

    void restoreRequiresConfirmationAndSave()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        Settings custom;
        custom.inputMethod = InputMethod::Vni;
        custom.toggleShortcut = "Ctrl+Shift+V";
        QVERIFY(SettingsStore(path).save(custom).isEmpty());
        SettingsWindow window(path);
        window.show();
        auto* defaults = window.findChild<QPushButton*>("defaults");
        defaults->click();
        QVERIFY(window.findChild<QPushButton*>("vni")->isChecked());
        defaults->click(); // Keep existing settings.
        QVERIFY(window.findChild<QPushButton*>("vni")->isChecked());
        defaults->click();
        window.findChild<QPushButton*>("pageAction")->click(); // Confirm restore.
        QVERIFY(window.findChild<QPushButton*>("telex")->isChecked());
        QVERIFY(SettingsStore(path).load().settings == custom); // Pending, not saved.
        window.findChild<QPushButton*>("save")->click();
        QVERIFY(SettingsStore(path).load().settings == Settings{});
    }

    void invalidShortcutPreventsSaving()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        SettingsWindow window(path);
        auto* shortcut = window.findChild<QKeySequenceEdit*>("shortcut");
        shortcut->setKeySequence(QKeySequence("A"));
        QVERIFY(!window.findChild<QPushButton*>("save")->isEnabled());
        QVERIFY(!QFile::exists(path));
        shortcut->setKeySequence(QKeySequence("Ctrl+Shift+V"));
        QVERIFY(window.findChild<QPushButton*>("save")->isEnabled());
        window.findChild<QPushButton*>("save")->click();
        QCOMPARE(SettingsStore(path).load().settings.toggleShortcut, std::string("Ctrl+Shift+V"));
    }

    void nativeShortcutLabelsKeepPortableConfiguration()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        SettingsWindow window(path);
        window.show();
        auto* display = window.findChild<QLineEdit*>("shortcutDisplay");
#ifdef Q_OS_MACOS
        QCOMPARE(display->text(), QStringLiteral("⌥Z"));
        QVERIFY(!window.findChild<QPushButton*>("suggestAlt")->text().contains("Alt"));
#else
        QVERIFY(display->text().contains("Alt"));
#endif
        window.findChild<QPushButton*>("save")->click();
        QCOMPARE(SettingsStore(path).load().settings.toggleShortcut, std::string("Alt+Z"));
        window.findChild<QKeySequenceEdit*>("shortcut")->setKeySequence(
            QKeySequence::fromString("Ctrl+Shift+V", QKeySequence::PortableText));
        window.findChild<QPushButton*>("save")->click();
        window.findChild<QPushButton*>("quickMenu")->click();
        auto* menuLabel = window.findChild<QLabel*>("menuShortcut");
        QVERIFY(menuLabel);
        QCOMPARE(menuLabel->text(), display->text());
#ifdef Q_OS_MACOS
        QVERIFY(display->text().contains(QStringLiteral("⌘")));
#endif
        menuLabel->window()->close();
        QCOMPARE(SettingsStore(path).load().settings.toggleShortcut, std::string("Ctrl+Shift+V"));
        SettingsWindow reopened(path);
        QCOMPARE(reopened.findChild<QLineEdit*>("shortcutDisplay")->text(), display->text());
    }

    void ioFailureIsReported()
    {
        QTemporaryDir dir;
        QFile blocker(dir.filePath("file"));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        SettingsStore store(dir.filePath("file/settings.json"));
        QVERIFY(!store.save(Settings{}).isEmpty());
    }

    void typingIsNotPersisted()
    {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.json");
        SettingsWindow window(path);
        window.show();
        window.findChild<QListWidget*>("navigation")->setCurrentRow(2);
        auto* typing = window.findChild<QTextEdit*>("typing");
        typing->setPlainText(QStringLiteral("Riêng tư 🐈"));
        QVERIFY(window.findChild<QLabel*>("characterCount")->text().startsWith("10"));
        window.findChild<QListWidget*>("navigation")->setCurrentRow(0);
        QTest::mouseClick(window.findChild<QPushButton*>("save"), Qt::LeftButton);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(!file.readAll().contains("Riêng"));
        SettingsWindow reopened(path);
        QVERIFY(reopened.findChild<QTextEdit*>("typing")->toPlainText().isEmpty());
    }

    void engineIsConnectedToSettingsAndCounter()
    {
        QTemporaryDir dir;
        SettingsWindow window(dir.filePath("settings.json"));
        window.show();
        auto* navigation = window.findChild<QListWidget*>("navigation");
        navigation->setCurrentRow(2);
        auto* editor = window.findChild<EngineTextEdit*>("typing");
        QVERIFY(editor);
        editor->setFocus();
        QTest::keyClicks(editor, "tieengs");
        QCOMPARE(editor->visibleText(), QStringLiteral("tiếng"));
        QCOMPARE(editor->preeditText(), QStringLiteral("tiếng"));
        QCOMPARE(window.findChild<QLabel*>("characterCount")->text(), QStringLiteral("5 ký tự"));
        QTest::keyClick(editor, Qt::Key_Space);
        QCOMPARE(editor->toPlainText(), QStringLiteral("tiếng "));
        navigation->setCurrentRow(0);
        QTest::mouseClick(window.findChild<QPushButton*>("vni"), Qt::LeftButton);
        navigation->setCurrentRow(2);
        editor->setFocus();
        QTest::keyClicks(editor, "Vie6t5 ");
        QCOMPARE(editor->toPlainText(), QStringLiteral("tiếng Việt "));
    }

    void editorDeletionRestoreAndNavigation()
    {
        EngineTextEdit editor;
        editor.show(); editor.setFocus();
        QTest::keyClicks(&editor, "tieengs");
        QTest::keyClick(&editor, Qt::Key_Backspace);
        QCOMPARE(editor.visibleText(), QStringLiteral("tiến"));
        QTest::keyClick(&editor, Qt::Key_Escape);
        QCOMPARE(editor.visibleText(), QStringLiteral("tieens"));
        QTest::keyClick(&editor, Qt::Key_Space);
        QTest::keyClicks(&editor, "dd");
        QTest::keyClick(&editor, Qt::Key_Left);
        QCOMPARE(editor.toPlainText(), QStringLiteral("tieens đ"));
        QVERIFY(editor.preeditText().isEmpty());
        QTest::keyClicks(&editor, "aas ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("tieens ấ đ"));
        QTest::keyClick(&editor, Qt::Key_Delete);
        QCOMPARE(editor.toPlainText(), QStringLiteral("tieens ấ "));
    }

    void editorPasteUndoSelectionAndClear()
    {
        EngineTextEdit editor;
        editor.show(); editor.setFocus();
        QTest::keyClicks(&editor, "aas");
        QTest::keySequence(&editor, QKeySequence::Undo);
        QCOMPARE(editor.visibleText(), QString());
        QTest::keySequence(&editor, QKeySequence::Redo);
        QCOMPARE(editor.toPlainText(), QStringLiteral("ấ"));
        QTest::keyClick(&editor, Qt::Key_Space);
        QTest::keyClicks(&editor, "dd");
        QApplication::clipboard()->setText(QStringLiteral("Tiếng Việt 😀"));
        editor.paste();
        QCOMPARE(editor.visibleText(), QStringLiteral("ấ đTiếng Việt 😀"));
        QVERIFY(editor.preeditText().isEmpty());
        editor.selectAll();
        QTest::keyClicks(&editor, "uws ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("ứ "));
        editor.clear();
        QTest::keyClicks(&editor, "dd");
        // Test through the base class too: callers can use the inherited slots.
        static_cast<QTextEdit*>(&editor)->clear();
        QCOMPARE(editor.visibleText(), QString());
        QTest::keyClicks(&editor, "tieengs ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("tiếng "));
        QTest::keyClicks(&editor, "as");
        static_cast<QTextEdit*>(&editor)->setPlainText(QStringLiteral("Mới"));
        QCOMPARE(editor.visibleText(), QStringLiteral("Mới"));
        auto cursor = editor.textCursor(); cursor.movePosition(QTextCursor::End); editor.setTextCursor(cursor);
        QTest::keyClicks(&editor, " dd ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("Mới đ "));
    }

    void editorExternalImeAndContextChanges()
    {
        EngineTextEdit editor;
        editor.show(); editor.setFocus();
        QTest::keyClicks(&editor, "dd");
        QInputMethodEvent foreign;
        foreign.setCommitString(QStringLiteral("Việt"));
        QApplication::sendEvent(&editor, &foreign);
        QCOMPARE(editor.toPlainText(), QStringLiteral("đViệt"));
        QVERIFY(editor.preeditText().isEmpty());
        editor.clear();
        QInputMethodEvent foreignPreedit(QStringLiteral("あ"), {});
        QApplication::sendEvent(&editor, &foreignPreedit);
        QCOMPARE(editor.visibleText(), QStringLiteral("あ"));
        QInputMethodEvent foreignCommit;
        foreignCommit.setCommitString(QStringLiteral("あ"));
        QApplication::sendEvent(&editor, &foreignCommit);
        QCOMPARE(editor.visibleText(), QStringLiteral("あ"));
        editor.clear();
        QTest::keyClicks(&editor, "tieengs");
        QFocusEvent blur(QEvent::FocusOut);
        QApplication::sendEvent(&editor, &blur);
        QCOMPARE(editor.toPlainText(), QStringLiteral("tiếng"));
        QVERIFY(editor.preeditText().isEmpty());
        Settings settings;
        settings.inputMethod = InputMethod::Vni;
        QTest::keyClicks(&editor, " dd");
        editor.configureEngine(settings);
        QCOMPARE(editor.toPlainText(), QStringLiteral("tiếng đ"));
        QTest::keyClicks(&editor, " a61 ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("tiếng đ ấ "));
        settings.enabled = false; editor.configureEngine(settings);
        QTest::keyClicks(&editor, "a61 ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("tiếng đ ấ a61 "));
        settings.enabled = true; editor.configureEngine(settings);
        QTest::keyClicks(&editor, "a61 ");
        QVERIFY(editor.toPlainText().endsWith(QStringLiteral("a61 ấ ")));
    }

    void editorProgrammaticCursorCommitsAtOriginalPosition()
    {
        EngineTextEdit editor;
        editor.show(); editor.setFocus();
        editor.setPlainText("AB ");
        auto cursor = editor.textCursor(); cursor.movePosition(QTextCursor::End); editor.setTextCursor(cursor);
        QTest::keyClicks(&editor, "aas");
        cursor = editor.textCursor(); cursor.setPosition(0); editor.setTextCursor(cursor);
        QCOMPARE(editor.toPlainText(), QStringLiteral("AB ấ"));
        QCOMPARE(editor.textCursor().position(), 0);
        QVERIFY(editor.preeditText().isEmpty());
        QTest::keyClicks(&editor, "dd ");
        QCOMPARE(editor.toPlainText(), QStringLiteral("đ AB ấ"));
    }
};

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VNInput");
    QCoreApplication::setApplicationName("VNInput");
    app.setStyle("Fusion");
    applyTheme(app);
    SettingsTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "settings_tests.moc"
