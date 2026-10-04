#include "settings_window.hpp"
#include "theme.hpp"
#include "controls.hpp"
#include <vninput/platform.hpp>

#include <QButtonGroup>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QResizeEvent>
#include <QStackedWidget>
#include "engine_text_edit.hpp"
#include <QTextBlockFormat>
#include <QTextCursor>
#include <utility>
#ifdef VNINPUT_MACOS_IME
#include "native_settings.hpp"
#endif

namespace vninput::ui {
namespace {
QString qstring(std::string_view value)
{ return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size())); }

QString shortcutLabel(const QString& portable)
{
    // NativeText supplies Option/Command glyphs on macOS; PortableText stays on disk.
    return QKeySequence::fromString(portable, QKeySequence::PortableText).toString(QKeySequence::NativeText);
}

QLabel* textAt(QWidget* parent, const QString& text, QRect rect, int size = 12,
              int weight = 400, const char* tone = "ink", bool mono = false)
{
    auto* label = new QLabel(text, parent);
    label->setGeometry(rect);
    label->setFont(uiFont(size, weight, mono));
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setProperty("tone", tone);
    label->setTextFormat(Qt::PlainText);
    return label;
}

QWidget* makePage(const QString& title, const QString& subtitle)
{
    auto* page = new QWidget;
    page->setObjectName("page");
    auto* heading = textAt(page, title, {22,20,461,24},19,650);
    auto font = heading->font(); font.setLetterSpacing(QFont::AbsoluteSpacing,0); heading->setFont(font);
    textAt(page, subtitle, {22,49,461,16},11,400,"muted");
    return page;
}

void lineAt(QWidget* parent, int y)
{
    auto* line = new QWidget(parent); line->setObjectName("divider"); line->setGeometry(22,y,461,1);
}

ActionButton* buttonAt(QWidget* parent, const QString& text, QRect rect, const char* name,
                       ActionButton::Kind kind = ActionButton::Standard)
{
    auto* button = new ActionButton(text,parent,kind);
    button->setGeometry(rect); button->setObjectName(name); return button;
}
} // namespace

SettingsWindow::SettingsWindow(QString configPath, QWidget* parent)
    : QWidget(parent), store_(std::move(configPath))
{
    setObjectName("window"); setWindowTitle(QStringLiteral("VNInput — Cài đặt"));
    setMinimumSize(640,480); resize(640,480);
    const auto loaded = store_.load(); saved_ = loaded.settings;
    loadBlocked_ = loaded.status == LoadStatus::Error;
    loadError_ = loaded.error;

    auto* header = new QWidget(this); header->setObjectName("header");
    headerLogo_ = new QLabel(header); headerLogo_->setGeometry(16,10,25,25);
    headerLogo_->setAccessibleName("Logo VNInput");
    auto* brand = textAt(header,"VNInput",{49,10,70,25},15,700);
    auto brandFont=brand->font(); brandFont.setLetterSpacing(QFont::AbsoluteSpacing,-0.5); brand->setFont(brandFont);
    modeDot_ = new QLabel(header); modeDot_->setObjectName("modeDot");
    modeLabel_ = textAt(header,{}, {0,0,56,16},11,400,"secondary");
    enabled_ = buttonAt(header,"V",{601,12,23,22},"enabled",ActionButton::Mode);
    enabled_->setCheckable(true);
    enabled_->setAccessibleName(QStringLiteral("Chuyển Việt / Anh"));
    enabled_->setToolTip(platform::currentBackend().implemented
        ? QStringLiteral("Lưu thay đổi để áp dụng cho bộ gõ hệ thống. Phím tắt V/E đổi chế độ tạm thời.")
        : QStringLiteral("Chế độ mặc định trong cấu hình. Adapter hệ thống chưa được triển khai."));

    auto* sidebar = new QWidget(this); sidebar->setObjectName("sidebar");
    navigation_ = new QListWidget(sidebar); navigation_->setObjectName("navigation");
    navigation_->setGeometry(8,14,118,190);
    navigation_->setItemDelegate(new NavigationDelegate(navigation_));
    navigation_->setFrameShape(QFrame::NoFrame);
    navigation_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigation_->setAccessibleName(QStringLiteral("Thiết lập VNInput"));
    navigation_->addItems({QStringLiteral("Bộ gõ"),QStringLiteral("Phím tắt"),QStringLiteral("Gõ thử"),
                           QStringLiteral("Hệ thống"),QStringLiteral("Về VNInput")});
    auto* menu = buttonAt(sidebar,QStringLiteral("Menu V/E         ↗"),{3,374,118,30},"quickMenu",ActionButton::Quiet);
    connect(menu,&QPushButton::clicked,this,&SettingsWindow::showQuickMenu);
    auto* version = textAt(sidebar,qstring(platform::currentBackend().operatingSystem)+QStringLiteral(" · v0.1"),
                          {17,406,106,15},10,400,"muted"); version->setObjectName("sidebarVersion");

    pages_ = new QStackedWidget(this); pages_->setObjectName("pages");
    pages_->addWidget(inputPage()); pages_->addWidget(shortcutPage()); pages_->addWidget(typingPage());
    pages_->addWidget(systemPage()); pages_->addWidget(aboutPage());
    auto* footer = new QWidget(this); footer->setObjectName("footer");
    feedback_ = textAt(footer,{}, {18,20,275,15},10,400,"muted"); feedback_->setObjectName("feedback");
    defaults_ = buttonAt(footer,QStringLiteral("Mặc định"),{318,13,72,30},"defaults",ActionButton::Quiet);
    save_ = buttonAt(footer,QStringLiteral("Lưu thay đổi"),{396,13,91,30},"save",ActionButton::Primary);
    pageAction_ = buttonAt(footer,{}, {344,13,143,30},"pageAction"); pageAction_->hide();
    connect(pageAction_,&QPushButton::clicked,this,[this] {
        if (pages_->currentIndex()==5) {
            loadBlocked_=false;
            showSettings(Settings{}); selectPage(navigation_->currentRow());
            feedback_->setText(QStringLiteral("Đã khôi phục mặc định"));
            return;
        }
        if (pages_->currentIndex()==2) navigation_->setCurrentRow(0);
        else if (pages_->currentIndex()==4) navigation_->setCurrentRow(3);
        else {
#ifdef VNINPUT_MACOS_IME
            const auto status = macos::inputSourceStatus();
            const auto message = status == macos::SourceStatus::Selected
                ? QStringLiteral("VNInput đang được chọn. Bạn có thể gõ tiếng Việt trong ứng dụng hỗ trợ bộ gõ macOS.")
                : status == macos::SourceStatus::Missing
                    ? QStringLiteral("Chưa tìm thấy bộ gõ VNInput đã cài. Cài VNInput vào thư mục Input Methods của tài khoản, rồi thêm nguồn nhập trong cài đặt Bàn phím.")
                    : QStringLiteral("Đã tìm thấy bộ gõ VNInput. Hãy chọn VNInput trong menu nguồn nhập của macOS; nếu chưa thấy, thêm VNInput trong cài đặt Bàn phím.");
            QMessageBox::information(this, QStringLiteral("Bộ gõ VNInput"), message);
#else
            QMessageBox::information(this,QStringLiteral("Bản scaffold"),
            QStringLiteral("Giao diện hướng dẫn đã sẵn sàng. Adapter ")+qstring(platform::currentBackend().inputApi)
            +QStringLiteral(" chưa được triển khai, nên VNInput chưa xuất hiện trong danh sách bộ gõ hệ thống."));
#endif
        }
    });
    connect(navigation_,&QListWidget::currentRowChanged,this,&SettingsWindow::selectPage);
    connect(telex_,&QPushButton::toggled,this,&SettingsWindow::changed);
    connect(vni_,&QPushButton::toggled,this,&SettingsWindow::changed);
    connect(enabled_,&QPushButton::toggled,this,&SettingsWindow::changed);
    connect(tone_,&QComboBox::currentIndexChanged,this,&SettingsWindow::changed);
    connect(freeTone_,&QCheckBox::toggled,this,&SettingsWindow::changed);
    connect(restore_,&QCheckBox::toggled,this,&SettingsWindow::changed);
    connect(shortcut_,&QKeySequenceEdit::keySequenceChanged,this,&SettingsWindow::changed);
    connect(save_,&QPushButton::clicked,this,[this] { save(); });
    connect(defaults_,&QPushButton::clicked,this,&SettingsWindow::restoreDefaults);
    showSettings(saved_); navigation_->setCurrentRow(0); refreshBrand();
}

void SettingsWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!pages_) return;
    findChild<QWidget*>("header")->setGeometry(0,0,width(),47);
    auto* sidebar = findChild<QWidget*>("sidebar"); sidebar->setGeometry(0,47,135,height()-47);
    enabled_->move(width()-39,12); modeLabel_->move(width()-103,15); modeDot_->setGeometry(width()-114,21,5,5);
    sidebar->findChild<QPushButton*>("quickMenu")->move(3,height()-106);
    sidebar->findChild<QLabel*>("sidebarVersion")->move(17,height()-74);
    pages_->setGeometry(135,47,width()-135,height()-102);
    auto* footer = findChild<QWidget*>("footer"); footer->setGeometry(135,height()-55,width()-135,55);
    defaults_->move(footer->width()-187,13); save_->move(footer->width()-109,13);
    pageAction_->move(footer->width()-18-pageAction_->width(),13);
}

QWidget* SettingsWindow::inputPage()
{
    auto* page=makePage(QStringLiteral("Bộ gõ"),QStringLiteral("Chọn cách gõ quen thuộc của bạn."));
    textAt(page,QStringLiteral("Kiểu gõ"),{22,87,107,18},12,550);
    auto* segment=new QWidget(page); segment->setObjectName("segmented"); segment->setGeometry(141,81,149,32);
    auto* group=new QButtonGroup(page);
    telex_=buttonAt(segment,"Telex",{3,3,77,26},"telex",ActionButton::Segment);
    vni_=buttonAt(segment,"VNI",{80,3,66,26},"vni",ActionButton::Segment);
    for(auto* button:{telex_,vni_}) { button->setCheckable(true); group->addButton(button); }
    methodHelp_=textAt(page,{}, {141,118,342,15},10,400,"muted");
    textAt(page,QStringLiteral("Ví dụ"),{22,142,107,17},11,400,"muted");
    example_=textAt(page,{}, {141,142,98,19},11,400,"muted",true);
    textAt(page,QStringLiteral("→"),{247,142,13,19},12,400,"muted");
    textAt(page,QStringLiteral("tiếng Việt"),{270,142,110,19},13,550);
    auto* toneLabel=textAt(page,QStringLiteral("Vị trí dấu"),{22,183,107,18},12,550);
    tone_=new ToneCombo(page); tone_->setObjectName("tone"); tone_->setGeometry(141,177,216,31);
    tone_->addItems({QStringLiteral("Kiểu mới · hoà, thuý"),QStringLiteral("Kiểu cũ · hòa, thúy")}); toneLabel->setBuddy(tone_);
    lineAt(page,226);
    freeTone_=new CheckBox(QStringLiteral("Bỏ dấu tự do"),page); freeTone_->setObjectName("freeTone");
    freeTone_->setGeometry(22,244,461,18);
    textAt(page,QStringLiteral("Đặt dấu trước hoặc sau phụ âm cuối."),{44,264,430,15},10,400,"muted");
    restore_=new CheckBox(QStringLiteral("Khôi phục từ gốc bằng Esc"),page); restore_->setObjectName("restoreOnEscape");
    restore_->setGeometry(22,292,461,18);
    textAt(page,QStringLiteral("Trả lại chuỗi phím khi từ vẫn đang soạn."),{44,313,430,15},10,400,"muted");
    textAt(page,QStringLiteral("Bảng mã"),{22,343,47,15},10,400,"muted");
    textAt(page,QStringLiteral("Unicode dựng sẵn"),{75,343,200,15},10,450,"secondary");
    return page;
}

QWidget* SettingsWindow::shortcutPage()
{
    auto* page=makePage(QStringLiteral("Phím tắt"),QStringLiteral("Chuyển ngôn ngữ, giữ mạch suy nghĩ."));
    auto* v=textAt(page,"V",{22,81,32,32},16,550,"accent"); v->setObjectName("letterTile"); v->setAlignment(Qt::AlignCenter);
    textAt(page,QStringLiteral("⇄"),{63,81,13,32},12,400,"muted");
    auto* e=textAt(page,"E",{85,81,32,32},16,550,"secondary"); e->setObjectName("secondaryTile"); e->setAlignment(Qt::AlignCenter);
    textAt(page,QStringLiteral("Chuyển nhanh Việt / Anh"),{130,81,300,32},11,400,"secondary");
    textAt(page,QStringLiteral("Phím chuyển"),{22,134,461,18},12,550);
    shortcut_=new QKeySequenceEdit(page); shortcut_->setObjectName("shortcut"); shortcut_->setGeometry(22,158,365,31);
    shortcut_->setMaximumSequenceLength(1); shortcut_->setAccessibleName(QStringLiteral("Phím chuyển"));
    shortcutDisplay_=new QLineEdit(page); shortcutDisplay_->setObjectName("shortcutDisplay");
    shortcutDisplay_->setGeometry(22,158,365,31); shortcutDisplay_->setReadOnly(true);
    shortcutDisplay_->setAccessibleName(QStringLiteral("Phím chuyển")); shortcutDisplay_->setFont(uiFont(11,400,true));
    shortcut_->hide();
    connect(shortcut_,&QKeySequenceEdit::editingFinished,this,[this] { shortcut_->hide(); shortcutDisplay_->show(); });
    auto* capture=buttonAt(page,QStringLiteral("Ghi phím tắt"),{395,158,88,30},"captureShortcut");
    connect(capture,&QPushButton::clicked,this,[this] { shortcutDisplay_->hide(); shortcut_->show(); shortcut_->setFocus(); });
    textAt(page,QStringLiteral("Nhấn Ghi phím tắt, rồi bấm tổ hợp mới."),{22,194,461,15},10,400,"muted");
    textAt(page,QStringLiteral("Gợi ý"),{22,222,31,20},10,400,"muted");
    auto* alt=buttonAt(page,shortcutLabel("Alt+Z"),{56,222,55,21},"suggestAlt");
    auto* ctrl=buttonAt(page,shortcutLabel("Ctrl+Shift+V"),{120,222,111,21},"suggestCtrl");
    connect(alt,&QPushButton::clicked,this,[this] { shortcut_->setKeySequence(QKeySequence("Alt+Z")); });
    connect(ctrl,&QPushButton::clicked,this,[this] { shortcut_->setKeySequence(QKeySequence("Ctrl+Shift+V")); });
#if defined(Q_OS_MACOS)
    textAt(page,QStringLiteral("⌥ Option · ⌃ Control · ⇧ Shift · ⌘ Command"),{22,245,461,15},10,400,"muted");
#else
    textAt(page,QStringLiteral("Ctrl, Alt và Shift là các phím bổ trợ."),{22,245,461,15},10,400,"muted");
#endif
    auto* details=buttonAt(page,QStringLiteral("▸ Phím hệ thống cần lưu ý"),{14,274,145,24},"shortcutDetails",ActionButton::Quiet);
    auto* note=textAt(page,shortcutLabel("Ctrl+Space")+QStringLiteral(" · Có thể trùng phím tắt hệ thống."),{22,311,461,30},10,400,"muted");
    note->hide(); connect(details,&QPushButton::clicked,note,[note] { note->setVisible(!note->isVisible()); });
    shortcut_->setToolTip(platform::currentBackend().implemented
        ? QStringLiteral("Phím chuyển V/E hoạt động khi nguồn nhập VNInput đang được chọn. Lưu để áp dụng.")
        : QStringLiteral("Chỉ lưu cấu hình; chưa đăng ký phím tắt toàn hệ thống."));
    return page;
}

QWidget* SettingsWindow::typingPage()
{
    auto* page=makePage(QStringLiteral("Gõ thử"),QStringLiteral("Thử dấu, sửa từ và làm quen kiểu gõ."));
    textAt(page,QStringLiteral("Viết vài dòng của bạn"),{22,81,300,19},12,550);
    auto* badge=textAt(page,"Telex",{444,81,39,19},10,400,"accent"); badge->setObjectName("methodBadge"); badge->setAlignment(Qt::AlignCenter);
    typing_=new EngineTextEdit(page); typing_->setObjectName("typing"); typing_->setGeometry(22,108,461,143);
    typing_->setAcceptRichText(false); typing_->setFont(uiFont(14)); typing_->document()->setDocumentMargin(12);
    typing_->setPlaceholderText(QStringLiteral("Bắt đầu gõ ở đây…"));
    typing_->setAccessibleName(QStringLiteral("Viết vài dòng của bạn"));
    typing_->setToolTip(QStringLiteral("Gõ bằng engine VNInput. Chọn nguồn nhập ABC/English của hệ điều hành để thử Telex hoặc VNI."));
    auto* count=textAt(page,QStringLiteral("0 ký tự"),{22,260,180,15},10,400,"muted"); count->setObjectName("characterCount");
    auto* sample=buttonAt(page,QStringLiteral("Điền câu mẫu"),{365,253,80,30},"sample",ActionButton::Quiet);
    auto* clear=buttonAt(page,QStringLiteral("Xoá"),{451,253,32,30},"clearSample",ActionButton::Quiet);
    connect(sample,&QPushButton::clicked,this,[this] { typing_->setPlainText(QStringLiteral("Tiếng Việt thật đẹp.\nGõ tự nhiên, viết điều bạn muốn.")); });
    connect(clear,&QPushButton::clicked,typing_,&QTextEdit::clear);
    connect(typing_,&EngineTextEdit::visibleTextChanged,page,[this,count] {
        count->setText(QStringLiteral("%1 ký tự").arg(typing_->visibleText().toUcs4().size()));
    });
    const QStringList keys{"aa","aw","ee","oo","ow","uw","dd"};
    const QStringList values{QStringLiteral("â"),QStringLiteral("ă"),QStringLiteral("ê"),QStringLiteral("ô"),QStringLiteral("ơ"),QStringLiteral("ư"),QStringLiteral("đ")};
    for(int i=0;i<7;++i) {
        auto* key=textAt(page,keys[i],{22+i*67,298,30,19},10,400,"muted",true); key->setObjectName(QString("reference%1").arg(i));
        textAt(page,values[i],{72+i*67,298,8,19},13);
        auto* rule=new QWidget(page); rule->setObjectName("divider"); rule->setGeometry(22+i*67,323,58,1);
    }
    auto* tones=textAt(page,"s f r x j",{22,334,90,15},10,400,"muted",true); tones->setObjectName("toneReference");
    textAt(page,QStringLiteral("sắc · huyền · hỏi · ngã · nặng"),{94,333,340,15},10,400,"muted");
    return page;
}

QWidget* SettingsWindow::systemPage()
{
    auto* page=makePage(QStringLiteral("Thiết lập hệ thống"),QStringLiteral("Để VNInput sẵn sàng trong các ứng dụng."));
    auto* notice=new QWidget(page); notice->setObjectName("notice"); notice->setGeometry(22,81,461,54);
    textAt(notice,QStringLiteral("Thêm VNInput vào hệ thống"),{12,10,430,16},11,550,"accent");
    textAt(notice,QStringLiteral("Thực hiện một lần sau khi cài đặt."),{12,28,430,16},11,400,"secondary");
    QStringList titles, details;
#if defined(Q_OS_MACOS)
    titles={QStringLiteral("Mở cài đặt Bàn phím"),QStringLiteral("Thêm nguồn nhập VNInput"),QStringLiteral("Chọn VNInput trên thanh menu")};
    details={QStringLiteral("Cài đặt hệ thống → Bàn phím → Nhập văn bản → Sửa."),QStringLiteral("Bấm dấu +, tìm Tiếng Việt và thêm VNInput."),QStringLiteral("Mở menu nguồn nhập và chọn VNInput.")};
#elif defined(Q_OS_WIN)
    titles={QStringLiteral("Mở cài đặt ngôn ngữ"),QStringLiteral("Thêm bàn phím VNInput"),QStringLiteral("Chọn VNInput để bắt đầu")};
    details={"Settings → Time & language → Language & region.",QStringLiteral("Language options của Tiếng Việt → thêm VNInput."),QStringLiteral("Nhấn Windows + Space và chọn VNInput.")};
#else
    titles={QStringLiteral("Cài VNInput và Fcitx 5"),QStringLiteral("Thêm phương thức nhập"),QStringLiteral("Chọn VNInput để bắt đầu")};
    details={QStringLiteral("Dùng gói VNInput dành cho bản phân phối của bạn."),QStringLiteral("Mở Fcitx 5 Configuration và thêm VNInput."),QStringLiteral("Chuyển sang VNInput trong menu Fcitx 5.")};
#endif
    for(int i=0;i<3;++i) {
        const int y=qRound(156.56+i*58.39);
        auto* number=textAt(page,QString::number(i+1),{22,y,21,21},10,400,"secondary"); number->setObjectName("stepNumber"); number->setAlignment(Qt::AlignCenter);
        textAt(page,titles[i],{54,y,429,18},12,550);
        textAt(page,details[i],{54,y+21,429,16},10,400,"muted");
    }
    // The visual guide matches the mock. The action explains scaffold status;
    // it never claims that a native IME was installed or became ready.
    configStatus_=textAt(page,{}, {22,334,461,32},10,400,"muted"); configStatus_->setObjectName("configStatus");
    configStatus_->setWordWrap(true); configStatus_->hide();
    return page;
}

QWidget* SettingsWindow::aboutPage()
{
    auto* page=makePage(QStringLiteral("Về VNInput"),QStringLiteral("Một bộ gõ nhỏ, dành cho tiếng Việt."));
    aboutLogo_=new QLabel(page); aboutLogo_->setGeometry(22,88,34,34); aboutLogo_->setAccessibleName("Logo VNInput");
    auto* brand=textAt(page,"VNInput",{64,88,150,34},20,700,"muted"); auto font=brand->font(); font.setLetterSpacing(QFont::AbsoluteSpacing,-.5); brand->setFont(font);
    textAt(page,QStringLiteral("0.1 · Bản xem trước"),{391,88,92,34},10,400,"muted");
    for (int i=0;i<2;++i) {
        auto* tagline=textAt(page,i==0?QStringLiteral("Gõ tự nhiên."):QStringLiteral("Viết tiếng mình."),{22,142+i*34,461,34},26,500);
        auto font=tagline->font(); font.setLetterSpacing(QFont::AbsoluteSpacing,-0.4); tagline->setFont(font);
    }
    const QStringList titles{QStringLiteral("Gọn gàng"),QStringLiteral("Riêng tư"),QStringLiteral("Quen thuộc")};
    const QStringList values{QStringLiteral("Chỉ mở thiết lập khi cần."),QStringLiteral("Xử lý trên thiết bị, không lưu nội dung gõ."),QStringLiteral("Telex và VNI, trên cả ba hệ điều hành.")};
    for(int i=0;i<3;++i) {
        textAt(page,titles[i],{22,234+i*29,74,16},11,550);
        textAt(page,values[i],{96,234+i*29,387,16},11,400,"muted");
    }
    page->setToolTip(platform::currentBackend().implemented
        ? QStringLiteral("C++20 / Qt Widgets. Bộ gõ macOS chạy bằng InputMethodKit và core độc lập với Qt.")
        : QStringLiteral("C++20 / Qt Widgets. Engine Telex/VNI chạy trong Gõ thử; adapter hệ điều hành chưa triển khai."));
    return page;
}

void SettingsWindow::selectPage(int index)
{
    pages_->setCurrentIndex(index);
    feedback_->show(); defaults_->setText(QStringLiteral("Mặc định")); defaults_->setFixedWidth(72);
    defaults_->move(width()-135-187,13);
    defaults_->setVisible(index<2); save_->setVisible(index<2); pageAction_->setVisible(index>=2);
    static_cast<ActionButton*>(pageAction_)->setKind(index==3?ActionButton::Primary:ActionButton::Standard);
    pageAction_->setFixedSize(index==2?83:index==3?125:127,30);
    pageAction_->move(width()-135-18-pageAction_->width(),13);
    if(index==2) pageAction_->setText(QStringLiteral("Đổi kiểu gõ"));
    if(index==3) pageAction_->setText(QStringLiteral("Tôi đã chọn VNInput"));
    if(index==4) pageAction_->setText(QStringLiteral("Hướng dẫn thiết lập"));
    changed();
}

void SettingsWindow::showQuickMenu()
{
    auto* menu=new QWidget(this,Qt::Popup); menu->setAttribute(Qt::WA_DeleteOnClose); menu->setObjectName("window"); menu->setProperty("quickMenu",true);
    menu->setFixedSize(272,302);
    auto* logo=new QLabel(menu); logo->setGeometry(14,16,22,22); logo->setPixmap(brandPixmap(22,devicePixelRatioF()));
    textAt(menu,"VNInput",{44,16,130,22},13,700);
    textAt(menu,vni_->isChecked()?"VNI":"Telex",{230,16,35,22},10,400,"muted");
    auto* menuShortcut=textAt(menu,shortcut_->keySequence().toString(QKeySequence::NativeText),{153,77,103,32},10,400,"muted",true);
    menuShortcut->setObjectName("menuShortcut");
    menuShortcut->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    menuShortcut->setAttribute(Qt::WA_TransparentForMouseEvents);
    textAt(menu,QStringLiteral("↗"),{247,231,12,32},12,400,"muted");
    textAt(menu,QStringLiteral("CHẾ ĐỘ GÕ"),{18,59,230,13},9,550,"muted");
    const QStringList rows{QStringLiteral("Tiếng Việt"),QStringLiteral("Tiếng Anh"),"Telex","VNI",QStringLiteral("Thiết lập VNInput…")};
    const int ys[]={77,109,154,186,231};
    for(int i=0;i<5;++i) {
        const bool checked=i==0?enabled_->isChecked():i==1?!enabled_->isChecked():i==2?telex_->isChecked():i==3?vni_->isChecked():false;
        textAt(menu,checked?QStringLiteral("✓"):QString(),{16,ys[i],16,32},12,400,"accent");
        auto* row=buttonAt(menu,rows[i],{35,ys[i],218,32},qPrintable(QString("menuRow%1").arg(i)),ActionButton::Menu);
        if (i==0) row->setFixedWidth(170);
        connect(row,&QPushButton::clicked,this,[this,menu,i] {
            if(i==0||i==1) enabled_->setChecked(i==0);
            else if(i==2) telex_->setChecked(true); else if(i==3) vni_->setChecked(true);
            else navigation_->setCurrentRow(0);
            menu->close();
        });
    }
    class MenuIcon : public QWidget {
    public: using QWidget::QWidget;
    protected: void paintEvent(QPaintEvent*) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing); drawNavIcon(p,0,QRectF(0,0,16,16),ink());
    }};
    auto* icon=new MenuIcon(menu); icon->setGeometry(16,239,16,16);
    for(int y:{147,224}) { auto* rule=new QWidget(menu); rule->setObjectName("divider"); rule->setGeometry(13,y,246,1); }
    menu->move(mapToGlobal(QPoint(14,140))); menu->show(); refreshBrand();
}
Settings SettingsWindow::currentSettings() const
{
    Settings settings;
    settings.inputMethod = vni_->isChecked() ? InputMethod::Vni : InputMethod::Telex;
    settings.tonePlacement = tone_->currentIndex() == 0 ? TonePlacement::Modern : TonePlacement::Classic;
    settings.enabled = enabled_->isChecked();
    settings.freeTone = freeTone_->isChecked();
    settings.restoreOnEscape = restore_->isChecked();
    settings.toggleShortcut = shortcut_->keySequence().toString(QKeySequence::PortableText).toStdString();
    return settings;
}

void SettingsWindow::showSettings(const Settings& settings)
{
    applying_ = true;
    telex_->setChecked(settings.inputMethod == InputMethod::Telex);
    vni_->setChecked(settings.inputMethod == InputMethod::Vni);
    tone_->setCurrentIndex(settings.tonePlacement == TonePlacement::Modern ? 0 : 1);
    enabled_->setChecked(settings.enabled);
    freeTone_->setChecked(settings.freeTone);
    restore_->setChecked(settings.restoreOnEscape);
    shortcut_->setKeySequence(QKeySequence::fromString(QString::fromStdString(settings.toggleShortcut), QKeySequence::PortableText));
    applying_ = false;
    changed();
}

bool SettingsWindow::dirty() const { return currentSettings() != saved_; }

void SettingsWindow::changed()
{
    if (applying_) return;
    if (typing_) typing_->configureEngine(currentSettings());
    enabled_->setText(enabled_->isChecked() ? "V" : "E");
    modeLabel_->setText(enabled_->isChecked() ? QStringLiteral("Tiếng Việt") : QStringLiteral("Tiếng Anh"));
    modeDot_->setStyleSheet(QString("background:%1;border-radius:2px;").arg(enabled_->isChecked() ? "#1d5d3b" : muted().name()));
    example_->setText(telex_->isChecked() ? "tieengs Vieetj" : "tie6ng1 Vie6t5");
    methodHelp_->setText(telex_->isChecked() ? QStringLiteral("Dùng chữ để thêm dấu.") : QStringLiteral("Dùng số để thêm dấu."));
    if (auto* badge=findChild<QLabel*>("methodBadge")) badge->setText(telex_->isChecked()?"Telex":"VNI");
    const QStringList keys=telex_->isChecked()?QStringList{"aa","aw","ee","oo","ow","uw","dd"}:QStringList{"a6","a8","e6","o6","o7","u7","d9"};
    for(int i=0;i<7;++i) findChild<QLabel*>(QString("reference%1").arg(i))->setText(keys[i]);
    findChild<QLabel*>("toneReference")->setText(telex_->isChecked()?"s f r x j":"1 2 3 4 5");
    shortcutDisplay_->setText(shortcut_->keySequence().toString(QKeySequence::NativeText));
    const bool valid = SettingsStore::validShortcut(shortcut_->keySequence().toString(QKeySequence::PortableText));
    save_->setEnabled(!loadBlocked_ && valid);
    if (loadBlocked_) {
        feedback_->setText(QStringLiteral("Lỗi cấu hình · xem Hệ thống"));
        feedback_->setToolTip(loadError_);
    } else if (!valid) {
        feedback_->setText(QStringLiteral("Kiểm tra lại phím tắt"));
    } else {
        feedback_->setText(dirty() ? QStringLiteral("Có thay đổi chưa lưu") : QStringLiteral("Áp dụng từ lần gõ tiếp theo"));
        feedback_->setToolTip(store_.path());
    }
    if (!loadBlocked_ && pages_->currentIndex()==2) feedback_->setText(QStringLiteral("Nội dung không được lưu lại."));
    if (!loadBlocked_ && pages_->currentIndex()==3) feedback_->setText(qstring(platform::currentBackend().operatingSystem));
    if (!loadBlocked_ && pages_->currentIndex()==4) feedback_->setText(QStringLiteral("macOS · Windows · Linux"));
    configStatus_->setVisible(loadBlocked_);
    configStatus_->setText(loadBlocked_
        ? loadError_ + QStringLiteral("\nDùng Mặc định để tạo lại cấu hình, hoặc sửa tệp rồi mở lại VNInput.")
        : QDir::toNativeSeparators(store_.path()));
    configStatus_->setToolTip(store_.path());
}

bool SettingsWindow::save()
{
    if (loadBlocked_) return false;
    const auto settings = currentSettings();
    const auto error = store_.save(settings);
    if (!error.isEmpty()) {
        feedback_->setText(QStringLiteral("Không thể lưu cấu hình"));
        feedback_->setToolTip(error);
        QMessageBox::warning(this, QStringLiteral("Chưa lưu được"), error);
        return false;
    }
    saved_ = settings;
    changed();
    feedback_->setText(QStringLiteral("Đã lưu thay đổi"));
    return true;
}

void SettingsWindow::restoreDefaults()
{
    if (pages_->currentIndex()==5) { selectPage(navigation_->currentRow()); return; }
    if (pages_->count()>5) { auto* old=pages_->widget(5); pages_->removeWidget(old); delete old; }
    auto* page=makePage(navigation_->currentRow()==0?QStringLiteral("Bộ gõ"):QStringLiteral("Phím tắt"),
        navigation_->currentRow()==0?QStringLiteral("Chọn cách gõ quen thuộc của bạn."):QStringLiteral("Chuyển ngôn ngữ, giữ mạch suy nghĩ."));
    auto* symbol=textAt(page,QStringLiteral("↺"),{22,144,461,45},31,400,"accent"); symbol->setAlignment(Qt::AlignCenter);
    auto* title=textAt(page,QStringLiteral("Khôi phục mặc định?"),{22,203,461,25},17,700); title->setAlignment(Qt::AlignCenter);
    auto* help=textAt(page,QStringLiteral("Kiểu gõ trở về Telex.\nPhím chuyển trở về %1.").arg(shortcutLabel(QString::fromStdString(Settings{}.toggleShortcut))),{22,244,461,44},12,400,"muted"); help->setAlignment(Qt::AlignCenter);
    if (loadBlocked_) help->setText(QStringLiteral("Tạo lại cấu hình mặc định.\nTệp cũ được thay thế khi bấm Lưu thay đổi."));
    pages_->addWidget(page); pages_->setCurrentIndex(5);
    feedback_->hide(); save_->hide(); defaults_->show(); pageAction_->show();
    defaults_->setText(QStringLiteral("Giữ thiết lập hiện tại")); defaults_->setGeometry(18,13,129,30);
    pageAction_->setText(QStringLiteral("Khôi phục")); pageAction_->setFixedSize(78,30);
    pageAction_->move(width()-135-18-78,13);
    static_cast<ActionButton*>(pageAction_)->setKind(ActionButton::Primary);
    refreshBrand();
}

void SettingsWindow::closeEvent(QCloseEvent* event)
{
    if (!dirty() || loadBlocked_) { event->accept(); return; }
    QMessageBox prompt(QMessageBox::Question, QStringLiteral("Thay đổi chưa lưu"),
        QStringLiteral("Lưu cấu hình trước khi đóng VNInput?"), QMessageBox::NoButton, this);
    auto* saveButton = prompt.addButton(QStringLiteral("Lưu"), QMessageBox::AcceptRole);
    auto* discardButton = prompt.addButton(QStringLiteral("Bỏ thay đổi"), QMessageBox::DestructiveRole);
    auto* cancelButton = prompt.addButton(QStringLiteral("Tiếp tục sửa"), QMessageBox::RejectRole);
    prompt.setDefaultButton(saveButton);
    prompt.setEscapeButton(cancelButton);
    prompt.exec();
    if (prompt.clickedButton() == discardButton || (prompt.clickedButton() == saveButton && save()))
        event->accept();
    else
        event->ignore();
}

void SettingsWindow::refreshBrand()
{
    headerLogo_->setPixmap(brandPixmap(25, devicePixelRatioF()));
    aboutLogo_->setPixmap(brandPixmap(34, devicePixelRatioF()));
    for (auto* label : findChildren<QLabel*>()) {
        const auto tone=label->property("tone").toString();
        if(tone.isEmpty()) continue;
        const auto color=tone=="muted"?muted():tone=="secondary"?secondary():tone=="accent"?accent():ink();
        label->setStyleSheet(QString("color:%1;").arg(color.name()));
    }
    setWindowIcon(QIcon(brandPixmap(128, devicePixelRatioF())));
}

} // namespace vninput::ui
