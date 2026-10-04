#pragma once

#include <vninput/settings.hpp>
#include <QString>

namespace vninput::ui {

enum class LoadStatus { Missing, Loaded, Error };

struct LoadResult {
    LoadStatus status = LoadStatus::Missing;
    Settings settings;
    QString error;
};

class SettingsStore {
public:
    explicit SettingsStore(QString path);
    static QString defaultPath();
    static bool validShortcut(const QString& shortcut);
    const QString& path() const { return path_; }
    LoadResult load() const;
    // Empty error means success. Saves atomically using QSaveFile.
    QString save(const Settings& settings) const;

private:
    QString path_;
};

} // namespace vninput::ui
