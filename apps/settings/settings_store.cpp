#include "settings_store.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QSaveFile>
#include <QStandardPaths>
#include <utility>
#ifdef VNINPUT_MACOS_IME
#include "native_settings.hpp"
#endif

namespace vninput::ui {
namespace {
constexpr int schemaVersion = 1;
constexpr qint64 maximumConfigBytes = 64 * 1024;

QString qstring(std::string_view value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

LoadResult failure(const QString& message)
{
    return {LoadStatus::Error, {}, message};
}
} // namespace

SettingsStore::SettingsStore(QString path) : path_(std::move(path)) {}

QString SettingsStore::defaultPath()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath("settings.json");
}

bool SettingsStore::validShortcut(const QString& shortcut)
{
#ifdef VNINPUT_MACOS_IME
    if (!macos::supportsShortcut(shortcut.toStdString())) return false;
#endif
    const auto sequence = QKeySequence::fromString(shortcut, QKeySequence::PortableText);
    if (sequence.count() != 1 || sequence[0].key() == Qt::Key_unknown
        || sequence[0].key() == 0)
        return false;
    const auto modifiers = sequence[0].keyboardModifiers();
    return modifiers.testFlag(Qt::ControlModifier) || modifiers.testFlag(Qt::AltModifier)
        || modifiers.testFlag(Qt::MetaModifier);
}

LoadResult SettingsStore::load() const
{
    QFile file(path_);
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly)) return failure(file.errorString());
    if (file.size() > maximumConfigBytes)
        return failure(QStringLiteral("Tệp cấu hình vượt quá 64 KB."));
    const auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) return failure(file.errorString());
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return failure(QStringLiteral("Tệp cấu hình không phải JSON hợp lệ."));
    const auto object = document.object();
    if (!object.value("schemaVersion").isDouble()
        || object.value("schemaVersion").toDouble() != schemaVersion)
        return failure(QStringLiteral("Phiên bản cấu hình chưa được hỗ trợ."));
    for (const auto* key : {"inputMethod", "tonePlacement", "toggleShortcut"}) {
        if (!object.value(key).isString())
            return failure(QStringLiteral("Thiếu hoặc sai kiểu trường %1.").arg(key));
    }
    for (const auto* key : {"enabled", "freeTone", "restoreOnEscape"}) {
        if (!object.value(key).isBool())
            return failure(QStringLiteral("Thiếu hoặc sai kiểu trường %1.").arg(key));
    }
    const auto method = inputMethodFromString(object.value("inputMethod").toString().toStdString());
    const auto tone = tonePlacementFromString(object.value("tonePlacement").toString().toStdString());
    if (!method || !tone) return failure(QStringLiteral("Kiểu gõ hoặc cách đặt dấu không hợp lệ."));

    Settings settings;
    settings.inputMethod = *method;
    settings.tonePlacement = *tone;
    settings.enabled = object.value("enabled").toBool();
    settings.freeTone = object.value("freeTone").toBool();
    settings.restoreOnEscape = object.value("restoreOnEscape").toBool();
    settings.toggleShortcut = object.value("toggleShortcut").toString().toStdString();
    if (validate(settings) || !validShortcut(QString::fromStdString(settings.toggleShortcut)))
        return failure(QStringLiteral("Phím tắt phải gồm một phím bổ trợ và một phím thường."));
    return {LoadStatus::Loaded, settings, {}};
}

QString SettingsStore::save(const Settings& settings) const
{
    if (validate(settings) || !validShortcut(QString::fromStdString(settings.toggleShortcut)))
        return QStringLiteral("Cấu hình không hợp lệ. Kiểm tra lại phím tắt.");
    const QFileInfo fileInfo(path_);
    if (!QDir().mkpath(fileInfo.absolutePath()))
        return QStringLiteral("Không thể tạo thư mục cấu hình.");
    const QJsonObject object{
        {"schemaVersion", schemaVersion},
        {"inputMethod", qstring(toString(settings.inputMethod))},
        {"tonePlacement", qstring(toString(settings.tonePlacement))},
        {"enabled", settings.enabled},
        {"freeTone", settings.freeTone},
        {"restoreOnEscape", settings.restoreOnEscape},
        {"toggleShortcut", QString::fromStdString(settings.toggleShortcut)}
    };
    const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Indented);
    QSaveFile file(path_);
    if (!file.open(QIODevice::WriteOnly)) return file.errorString();
    if (file.write(bytes) != bytes.size()) return file.errorString();
    if (!file.commit()) return file.errorString();
#ifdef VNINPUT_MACOS_IME
    if (QDir::cleanPath(path_) == QDir::cleanPath(defaultPath())) macos::notifySettingsChanged();
#endif
    return {};
}

} // namespace vninput::ui
