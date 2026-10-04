#pragma once

#include "settings_store.hpp"
#include <QWidget>

class QCheckBox;
class QCloseEvent;
class QComboBox;
class QKeySequenceEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;

namespace vninput::ui {

class EngineTextEdit;

class SettingsWindow final : public QWidget {
    Q_OBJECT
public:
    explicit SettingsWindow(QString configPath = SettingsStore::defaultPath(), QWidget* parent = nullptr);
    void refreshBrand();

protected:
    void closeEvent(QCloseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget* inputPage();
    QWidget* shortcutPage();
    QWidget* typingPage();
    QWidget* systemPage();
    QWidget* aboutPage();
    Settings currentSettings() const;
    void showSettings(const Settings& settings);
    void changed();
    bool save();
    void restoreDefaults();
    void selectPage(int index);
    void showQuickMenu();
    bool dirty() const;

    SettingsStore store_;
    Settings saved_;
    bool applying_ = false;
    bool loadBlocked_ = false;
    QString loadError_;
    QListWidget* navigation_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QPushButton* telex_ = nullptr;
    QPushButton* vni_ = nullptr;
    QPushButton* enabled_ = nullptr;
    QComboBox* tone_ = nullptr;
    QCheckBox* freeTone_ = nullptr;
    QCheckBox* restore_ = nullptr;
    QKeySequenceEdit* shortcut_ = nullptr;
    QLineEdit* shortcutDisplay_ = nullptr;
    EngineTextEdit* typing_ = nullptr;
    QLabel* example_ = nullptr;
    QLabel* feedback_ = nullptr;
    QLabel* headerLogo_ = nullptr;
    QLabel* aboutLogo_ = nullptr;
    QLabel* configStatus_ = nullptr;
    QPushButton* save_ = nullptr;
    QPushButton* defaults_ = nullptr;
    QPushButton* pageAction_ = nullptr;
    QLabel* modeLabel_ = nullptr;
    QLabel* modeDot_ = nullptr;
    QLabel* methodHelp_ = nullptr;
};

} // namespace vninput::ui
