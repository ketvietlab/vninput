#include "settings_window.hpp"
#include "theme.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QStyleHints>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("VNInput");
    QCoreApplication::setApplicationName("VNInput");
    QCoreApplication::setApplicationVersion(VNINPUT_VERSION);
    QApplication::setApplicationDisplayName("VNInput");
    QApplication::setQuitOnLastWindowClosed(true);
    QApplication::setStyle("Fusion");

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("VNInput — Cài đặt bộ gõ tiếng Việt"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"config", QStringLiteral("Đường dẫn tệp cấu hình JSON."), "path"});
    parser.process(app);

    vninput::ui::applyTheme(app);
    const auto path = parser.isSet("config") ? QDir::cleanPath(parser.value("config"))
        : vninput::ui::SettingsStore::defaultPath();
    vninput::ui::SettingsWindow window(path);
    QObject::connect(app.styleHints(), &QStyleHints::colorSchemeChanged, &window, [&] {
        vninput::ui::applyTheme(app);
        window.refreshBrand();
    });
    window.show();
    return app.exec();
}
