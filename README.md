# VNInput

Bộ gõ tiếng Việt bằng **C++20**, **CMake**, **Qt 6 Widgets** cho macOS,
Windows và Linux. Engine Telex/VNI và giao diện cài đặt đã chạy trong ô **Gõ thử**;
adapter macOS InputMethodKit đã có bản thử nghiệm. Windows/Linux vẫn là scaffold.
Hướng dẫn và giới hạn bản native: [docs/macos.md](docs/macos.md).

## Chạy trên máy phát triển

Cần CMake **3.24+**, Ninja, compiler hỗ trợ C++20 và Qt **6.5+** với Widgets/Test.
Không cần Node.js để build hay chạy ứng dụng Qt.

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Nếu CMake không tìm thấy Qt, thêm `-DCMAKE_PREFIX_PATH=/đường/dẫn/Qt/6.x/kit`
khi configure. Windows: chạy trong Developer PowerShell của Visual Studio,
dùng Qt kit tương thích compiler và thêm thư mục `bin` của Qt vào `PATH`.

Chạy ứng dụng theo hệ điều hành:

```sh
# macOS
open build/dev/apps/settings/vninput-settings.app

# Linux
./build/dev/apps/settings/vninput-settings

# Windows / PowerShell
.\build\dev\apps\settings\vninput-settings.exe
```

Ứng dụng mở ở **640 × 480**; menu V/E **272 × 302**. Đóng cửa sổ sẽ thoát
tiến trình cài đặt. Cửa sổ hỏi lưu nếu có thay đổi chưa lưu.

## Đã có

- Bộ gõ: Telex/VNI, vị trí dấu, bỏ dấu tự do, Esc khôi phục từ gốc, chế độ V/E.
- Phím tắt: ghi một tổ hợp mới, gợi ý, kiểm tra định dạng và lưu cấu hình.
- Gõ thử: engine Telex/VNI thật, preedit/commit, đếm Unicode code point, điền câu mẫu,
  xoá, paste và undo/redo; không lưu nội dung.
- Hướng dẫn hệ thống theo OS, giới thiệu, menu nhanh và xác nhận khôi phục mặc định.
- JSON cấu hình có phiên bản, kiểm tra dữ liệu, ghi nguyên tử bằng `QSaveFile`.
- Tách core C++ khỏi Qt; build riêng core và metadata nền tảng bằng preset `core-only`.
- Màu sáng/tối từ Két DS; bố cục và widget riêng của VNInput, logo nhúng vào Qt resource.

Chọn nguồn nhập **ABC/English** của hệ điều hành, mở **Gõ thử**, gõ
`tieengs Vieetj` (Telex) hoặc `tie6ng1 Vie6t5` (VNI). Các lựa chọn Bộ gõ áp dụng
ngay trong ô thử; **Lưu thay đổi** ghi cấu hình cho lần mở sau. Esc trả lại phím
gốc của từ đang nhập. Chi tiết quy tắc và giới hạn: [docs/engine.md](docs/engine.md).

Trên macOS, nút xác nhận thiết lập đọc trạng thái nguồn nhập thật; adapter riêng
cung cấp menu và phím chuyển V/E khi đang chọn VNInput. Bản thử nghiệm có script
cài cho tài khoản hiện tại và ký ad-hoc, chưa notarize hay có installer phát hành.
Kiểm thử tự động chưa chứng minh được độ ổn định khi gõ vào ứng dụng khác.
Windows/Linux chưa có adapter bộ gõ native.

## Cấu hình

Tệp `settings.json` nằm dưới `QStandardPaths::AppConfigLocation`, định danh ứng dụng
`VNInput/VNInput`. Tooltip ở dòng trạng thái màn Bộ gõ cho biết đường dẫn cụ thể.
Nhãn phím dùng cách viết native của hệ điều hành: macOS hiển thị `⌥Z` (Option + Z),
`⇧⌘V` và các ký hiệu tương ứng. Tệp cấu hình vẫn dùng Qt PortableText; formatter
không thay đổi tổ hợp đã lưu. Menu V/E lấy nhãn từ phím tắt đang chọn.
Schema hiện tại: `schemaVersion: 1`. Lỗi JSON, phiên bản lạ hoặc dữ liệu sai sẽ chặn
lưu để tránh ghi đè âm thầm. Chọn **Mặc định → Khôi phục → Lưu thay đổi** để tạo lại,
hoặc sửa tệp rồi mở lại ứng dụng.

Có thể dùng một tệp riêng khi phát triển:

```sh
QT_QPA_PLATFORM=offscreen ./build/dev/apps/settings/vninput-settings.app/Contents/MacOS/vninput-settings --version
./build/dev/apps/settings/vninput-settings.app/Contents/MacOS/vninput-settings --config /tmp/vninput-demo/settings.json
```

## Cấu trúc

```text
core/                   Engine Telex/VNI, kiểu dữ liệu, kiểm tra cấu hình; không Qt
platforms/              Chọn backend theo OS; hiện chỉ có metadata, chưa có IME
  macos/                Điểm bắt đầu cho InputMethodKit / Objective-C++
  windows/              Điểm bắt đầu cho Text Services Framework (TSF)
  linux/                Điểm bắt đầu cho Fcitx 5
apps/settings/          Qt Widgets, JSON store, giao diện và bộ điều khiển
resources/branding/     Logo nhúng trong executable
tests/                  Engine, persistence, tương tác Qt và công cụ chụp UI
design/                 Mock KetAtlas/KetJS gốc, không là dependency của app Qt
design/evidence/qt/     Ảnh reference/candidate/diff và kết quả KetPlus
```

Chi tiết ranh giới module và bước phát triển tiếp: [docs/architecture.md](docs/architecture.md).

## Build Release và cài vào thư mục staging

```sh
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix build/stage
```

Mặc định staging sử dụng Qt đã cài trên máy. Với macOS/Windows, bật
`-DVNINPUT_DEPLOY_QT=ON` khi configure để chạy deployment script của Qt khi install.
Đây chưa phải installer đã ký. Trên Linux, chọn cách đóng gói và các dependency
theo distro trước khi phát hành.

```sh
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only
```

## Đối chiếu với mockup

Ngưỡng người dùng chấp nhận: **3%**. KetPlus `visual_compare` đã kiểm tra toàn bộ
vùng nội dung, không mask/crop, `pixelThreshold: 0.1`, light, DPR 1. Kết quả trên
macOS arm64 / Qt 6.11.1:

| Màn hình | Pixel sai khác |
| --- | ---: |
| Bộ gõ | 1,80% |
| Phím tắt | 1,68% |
| Gõ thử | 1,94% |
| Thiết lập hệ thống | 2,24% |
| Về VNInput | 1,66% |
| Khôi phục mặc định | 1,07% |
| Menu V/E | 1,43% |

[Báo cáo máy đọc được](design/evidence/qt/comparison.json) ·
[Ảnh ứng dụng Qt](design/evidence/qt/candidate/settings.png) ·
[Mock gốc](design/evidence/qt/reference/settings.png).

Ba màn có phím tắt đã được đối chiếu lại sau yêu cầu sửa tên phím macOS:
[báo cáo mới](design/evidence/mac-shortcuts/comparison.json).
Mock được sửa nội dung phím theo yêu cầu này; báo cáo bảy màn trước đó được giữ
làm lịch sử, không ghi đè evidence cũ.

Qt được chụp trực tiếp bằng `QWidget::grab()` qua plugin `offscreen`. Vì KetPlus
so sánh trang web, ảnh native và ảnh mock được hiển thị nguyên kích thước trong
hai HTML shell giống nhau. Kết quả này đo **raster**; không coi cây ảnh HTML là
kiểm tra accessibility hay tương tác của widget native. Tương tác và persistence
được kiểm tra riêng bằng Qt Test; accessibility cần nghiệm thu riêng. Chưa đo
dark mode, Windows/Linux, DPI khác hoặc mọi trạng
thái trong Atlas. Không thay đổi mock để làm phép so sánh pass.

Quy trình tái kiểm tra: [tests/visual/README.md](tests/visual/README.md).

## Dependency và tài nguyên

Qt Core/Gui/Widgets; Qt Test chỉ dùng khi bật `BUILD_TESTING`. Không dùng Qt Quick,
QML hoặc Qt WebEngine. Token màu từ `@ketvietlab/design-system@0.1.29`; không nhúng
runtime web của design system. [Thông tin bên thứ ba](THIRD_PARTY_NOTICES.md).

## Giấy phép

VNInput là mã nguồn mở theo [giấy phép MIT](LICENSE), Copyright (c) 2026 KETVIET JSC, Vietnam.
Giấy phép áp dụng cho toàn bộ repo: lõi C++, adapter từng nền tảng, công cụ, tài liệu và mock trong `design/`.

- Tên "VNInput" và logo là nhãn hiệu của KETVIET JSC. Giấy phép MIT không cấp quyền dùng nhãn hiệu: bản fork
  phát hành lại phải đổi tên và logo, nhưng vẫn được ghi "dựa trên VNInput".
- Qt được liên kết động theo giấy phép của bản Qt bạn cài (LGPL-3.0 hoặc thương mại). Ai phát hành binary kèm Qt
  phải tuân thủ giấy phép đó.
- Thành phần bên thứ ba: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
