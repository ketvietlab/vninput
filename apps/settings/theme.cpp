#include "theme.hpp"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QPalette>
#include <QStyleHints>

namespace vninput::ui {

void applyTheme(QApplication& app)
{
    // Két Design System 0.1.29 semantic colors. Layout is VNInput-specific.
    const bool dark = app.styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    const QString panel = dark ? "#1d2228" : "#ffffff";
    const QString sidebar = dark ? "#171b20" : "#f7f5f5";
    const QString text = dark ? "#f2f4f7" : "#24262a";
    const QString secondary = dark ? "#b4bac4" : "#5a5c5e";
    const QString muted = dark ? "#858d99" : "#717373";
    const QString disabled = dark ? "#505862" : "#9a9ea2";
    const QString border = dark ? "rgba(255,255,255,22)" : "#dddcde";
    const QString accent = dark ? "#5968df" : "#4f5ed0";
    const QString hover = dark ? "#7485e8" : "#4557bc";
    const QString subtle = dark ? "rgba(89,104,223,36)" : "#eef0fb";

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(panel));
    palette.setColor(QPalette::Base, QColor(panel));
    palette.setColor(QPalette::AlternateBase, QColor(sidebar));
    palette.setColor(QPalette::Button, QColor(panel));
    palette.setColor(QPalette::WindowText, QColor(text));
    palette.setColor(QPalette::Text, QColor(text));
    palette.setColor(QPalette::ButtonText, QColor(text));
    palette.setColor(QPalette::PlaceholderText, QColor(muted));
    palette.setColor(QPalette::Highlight, QColor(accent));
    palette.setColor(QPalette::HighlightedText, Qt::white);
    palette.setColor(QPalette::ToolTipBase, QColor(panel));
    palette.setColor(QPalette::ToolTipText, QColor(text));
    for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
        palette.setColor(QPalette::Disabled, role, QColor(disabled));
    app.setPalette(palette);

#ifdef Q_OS_MACOS
    static const int monoId=QFontDatabase::addApplicationFont("/System/Library/Fonts/SFNSMono.ttf");
    const auto families=QFontDatabase::applicationFontFamilies(monoId);
    app.setProperty("monoFamily", families.isEmpty()?QString("Monaco"):families.first());
#else
    app.setProperty("monoFamily", QFontDatabase::systemFont(QFontDatabase::FixedFont).family());
#endif
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
#ifdef Q_OS_MACOS
    font.setFamily(".AppleSystemUIFont");
#endif
    font.setPixelSize(12);
    app.setFont(font);
    QString style = QStringLiteral(R"(
        QWidget { color: @text; }
        QWidget#window, QWidget#page, QStackedWidget { background: @panel; }
        QWidget#header { background: @panel; border-bottom: 1px solid @divider; }
        QWidget#sidebar { background: @sidebar; border-right: 1px solid @divider; }
        QListWidget#navigation { background: transparent; border: none; outline: none; padding: 0; }
        QWidget#footer { background: @panel; border-top: 1px solid @divider; }
        QWidget#divider { background: @divider; }
        QWidget#segmented { background: @sidebar; border: 1px solid @border; border-radius: 6px; }
        QWidget#notice, QLabel#methodBadge { background: @subtle; border-radius: 5px; }
        QLabel#letterTile { background: @subtle; border: 1px solid @accentBorder; border-radius: 6px; }
        QLabel#secondaryTile, QLabel#stepNumber { background: @sidebar; border: 1px solid @border; border-radius: 6px; }
        QLabel#stepNumber { border-radius: 10px; }
        QKeySequenceEdit { background: @panel; border: 1px solid @border; border-radius: 5px; padding: 0 9px; }
        QLineEdit#shortcutDisplay { background: @panel; border: 1px solid @border; border-radius: 5px; padding: 0 9px; }
        QKeySequenceEdit QLineEdit { background: transparent; border: none; font-size: 11px; font-family: "@monoFamily"; }
        QTextEdit { background: @panel; border: 1px solid @border; border-radius: 6px; padding: 0; }
        QTextEdit:focus, QKeySequenceEdit:focus { border: 1px solid @accent; }
        QComboBox QAbstractItemView { background: @panel; selection-background-color: @subtle; selection-color: @text; }
        QToolTip { background: @panel; color: @text; border: 1px solid @border; padding: 5px; }
    )");
    style.replace("@monoFamily", app.property("monoFamily").toString());
    style.replace("@accentBorder", dark ? "rgba(89,104,223,82)" : "#c3cdf0");
    for (const auto& pair : {std::pair{QString("@panel"), panel}, {"@sidebar", sidebar},
             {"@text", text}, {"@secondary", secondary}, {"@muted", muted},
             {"@disabled", disabled}, {"@border", border}, {"@accent", accent},
             {"@hover", hover}, {"@subtle", subtle},
             {"@divider", dark ? QString("rgba(255,255,255,14)") : QString("#e9e7e8")},
             {"@accentBorder", dark ? QString("rgba(89,104,223,82)") : QString("#c3cdf0")}})
        style.replace(pair.first, pair.second);
    app.setStyleSheet(style);
}

QPixmap brandPixmap(int logicalSize, qreal devicePixelRatio)
{
    QPixmap source(":/vninput/branding/vninput-logo.png");
    if (source.isNull()) return {};
    const int pixels = qRound(logicalSize * devicePixelRatio);
    auto result = source.scaled(pixels, pixels, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&result);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), qApp->palette().color(QPalette::Highlight));
    painter.end();
    result.setDevicePixelRatio(devicePixelRatio);
    return result;
}

} // namespace vninput::ui
