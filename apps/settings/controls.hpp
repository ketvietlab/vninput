#pragma once

#include <QApplication>
#include <QFocusEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyledItemDelegate>

namespace vninput::ui {

inline QColor ink() { return qApp->palette().color(QPalette::Text); }
inline bool darkTheme() { return qApp->palette().color(QPalette::Window).lightness() < 128; }
inline QColor panel() { return qApp->palette().color(QPalette::Window); }
inline QColor muted() { return QColor(darkTheme() ? "#858d99" : "#717373"); }
inline QColor secondary() { return QColor(darkTheme() ? "#b4bac4" : "#5a5c5e"); }
inline QColor accent() { return qApp->palette().color(QPalette::Highlight); }
inline QColor border() { return darkTheme() ? QColor(255,255,255,22) : QColor("#dddcde"); }
inline QColor divider() { return darkTheme() ? QColor(255,255,255,14) : QColor("#e9e7e8"); }
inline QColor subtle() { return darkTheme() ? QColor(89,104,223,36) : QColor("#eef0fb"); }
inline QColor sidebarColor() { return QColor(darkTheme() ? "#171b20" : "#f7f5f5"); }

inline QFont uiFont(int size = 12, int weight = 400, bool mono = false)
{
    QFont font = QApplication::font();
    if (mono) font.setFamily(qApp->property("monoFamily").toString());
    font.setPixelSize(size);
    font.setWeight(static_cast<QFont::Weight>(weight));
    if (!mono && size <= 13) font.setLetterSpacing(QFont::AbsoluteSpacing,-0.16);
    return font;
}

inline void drawNavIcon(QPainter& painter, int index, const QRectF& rect, const QColor& color)
{
    painter.save();
    painter.translate(rect.topLeft());
    painter.scale(rect.width() / 24, rect.height() / 24);
    painter.setPen(QPen(color, 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    QPainterPath path;
    if (index == 0) {
        painter.drawRoundedRect(QRectF(3,5,18,14),3,3);
        for (const auto& point : {QPointF(7,9),QPointF(11,9),QPointF(15,9),QPointF(19,9),
                 QPointF(7,13),QPointF(11,13),QPointF(15,13)}) painter.drawPoint(point);
        path.moveTo(7,16); path.lineTo(17,16);
    } else if (index == 1) {
        path.moveTo(13,3); path.lineTo(4,14); path.lineTo(11,14); path.lineTo(10,21);
        path.lineTo(20,9); path.lineTo(13,9); path.closeSubpath();
    } else if (index == 2) {
        path.moveTo(4,20); path.lineTo(8,19); path.lineTo(20,7); path.lineTo(17,4);
        path.lineTo(5,16); path.closeSubpath(); path.moveTo(14,7); path.lineTo(17,10);
    } else if (index == 3) {
        painter.drawRoundedRect(QRectF(3,3,18,13),2,2);
        path.moveTo(8,21); path.lineTo(16,21); path.moveTo(12,16); path.lineTo(12,21);
        path.moveTo(9,10); path.lineTo(11,12); path.lineTo(15,8);
    } else {
        painter.drawEllipse(QRectF(3,3,18,18));
        path.moveTo(12,11); path.lineTo(12,17); painter.drawPoint(QPointF(12,7));
    }
    painter.drawPath(path);
    painter.restore();
}

class NavigationDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex& index) const override
    { return {118, index.row() == 3 ? 51 : index.row() == 4 ? 32 : 35}; }
    void paint(QPainter* p, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        p->save(); p->setRenderHint(QPainter::Antialiasing);
        const bool selected = option.state.testFlag(QStyle::State_Selected);
        QRectF r = option.rect; r.setTop(r.top() + (index.row() == 3 ? 16 : 0)); r.setHeight(32);
        p->setPen(Qt::NoPen);
        if (selected) { p->setBrush(subtle()); p->drawRoundedRect(r,5,5); }
        const auto color = selected ? accent() : secondary();
        drawNavIcon(*p, index.row(), QRectF(r.x()+9,r.y()+8,16,16), color);
        p->setPen(color); p->setFont(uiFont(12, selected ? 600 : 400));
        p->drawText(QRectF(r.x()+33,r.y(),r.width()-34,32),Qt::AlignVCenter,index.data().toString());
        p->restore();
    }
};

class ActionButton : public QPushButton {
public:
    enum Kind { Standard, Primary, Quiet, Segment, Mode, Menu };
    explicit ActionButton(const QString& title, QWidget* parent, Kind kind = Standard)
        : QPushButton(title,parent), kind_(kind) { setCursor(Qt::PointingHandCursor); }
    void setKind(Kind kind) { kind_ = kind; update(); }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QRectF r(0.5,0.5,width()-1,height()-1);
        QColor bg = panel(), fg = ink(), edge = border();
        int weight = 400, size = 11;
        if (kind_ == Primary) { bg = accent(); fg = Qt::white; edge = bg; weight = 550; }
        if (kind_ == Quiet) { bg = Qt::transparent; edge = Qt::transparent; fg = secondary(); }
        if (kind_ == Menu) { bg = Qt::transparent; edge = Qt::transparent; size = 12; }
        if (kind_ == Segment) {
            bg = isChecked() ? panel() : Qt::transparent; edge = Qt::transparent;
            fg = isChecked() ? accent() : secondary(); size = 12; weight = isChecked() ? 600 : 400;
        }
        if (kind_ == Mode) { size = 12; weight = 600; }
        if (underMouse() && kind_ != Primary && kind_ != Segment) bg = sidebarColor();
        if (!isEnabled()) fg = muted();
        p.setPen(QPen(edge,1)); p.setBrush(bg); p.drawRoundedRect(r,kind_ == Mode ? 4 : 5,kind_ == Mode ? 4 : 5);
        p.setFont(uiFont(size,weight)); p.setPen(fg);
        p.drawText(QRectF(kind_ == Menu ? 6 : 0,0,width()-(kind_ == Menu ? 6 : 0),height()),
                   kind_ == Menu ? Qt::AlignLeft | Qt::AlignVCenter : Qt::AlignCenter,text());
        if (hasFocus() && focusReason_) {
            p.setPen(QPen(accent(),1)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(r.adjusted(2,2,-2,-2),3,3);
        }
    }
    void focusInEvent(QFocusEvent* event) override
    {
        focusReason_ = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason;
        QPushButton::focusInEvent(event);
    }
private:
    Kind kind_;
    bool focusReason_ = false;
};

class CheckBox final : public QCheckBox {
public:
    using QCheckBox::QCheckBox;
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(isChecked() ? accent() : border(),1));
        p.setBrush(isChecked() ? accent() : panel());
        p.drawRoundedRect(QRectF(0.5,2.5,13,13),2,2);
        if (isChecked()) {
            p.setPen(QPen(Qt::white,1.7,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            QPainterPath tick; tick.moveTo(3,9); tick.lineTo(5.6,11.5); tick.lineTo(11,5.8); p.drawPath(tick);
        }
        p.setFont(uiFont(12,500)); p.setPen(ink());
        p.drawText(QRectF(22,0,width()-22,17.4),Qt::AlignVCenter,text());
        if (hasFocus()) { p.setPen(QPen(accent(),1,Qt::DotLine)); p.setBrush(Qt::NoBrush); p.drawRect(rect().adjusted(0,0,-1,-1)); }
    }
};

class ToneCombo final : public QComboBox {
public:
    using QComboBox::QComboBox;
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(hasFocus() ? accent() : border(),1)); p.setBrush(panel());
        p.drawRoundedRect(QRectF(.5,.5,width()-1,height()-1),5,5);
        p.setFont(uiFont(12)); p.setPen(ink());
        p.drawText(QRectF(13,0,width()-28,height()),Qt::AlignVCenter,currentText());
        p.setPen(QPen(ink(),1.3,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        QPainterPath path; path.moveTo(width()-11,13); path.lineTo(width()-7,17); path.lineTo(width()-3,13); p.drawPath(path);
    }
};

} // namespace vninput::ui
