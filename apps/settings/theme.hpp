#pragma once

#include <QPixmap>

class QApplication;

namespace vninput::ui {
void applyTheme(QApplication& app);
QPixmap brandPixmap(int logicalSize, qreal devicePixelRatio = 1.0);
} // namespace vninput::ui
