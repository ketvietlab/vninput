# macOS InputMethodKit — bản thử nghiệm

Adapter Objective-C++ dùng engine C++ riêng cho mỗi input context, không phụ
thuộc Qt. Settings ghi JSON dùng chung; adapter đọc bằng Foundation. Phím chuyển
V/E chỉ được xử lý khi nguồn nhập VNInput đang được chọn. Build hiện yêu cầu macOS
14 trở lên; bundle cài cục bộ ký ad-hoc, chưa notarize để phát hành.

Core, Qt và mock IMK client đã qua kiểm thử tự động. Bài kiểm tra nhập liệu vào
NSTextView thật chưa đạt: sự kiện tổng hợp cho ra `tiees`, chưa xác định chắc chắn
đó là giới hạn của bộ kiểm tra hay adapter. Chưa coi đây là bản ổn định để dùng
hằng ngày. Không dùng lại mã OpenKey.

## Build và cài riêng adapter

```sh
cmake --preset release
cmake --build --preset release --target vninput-input-method vninput-input-source
cmake --install build/release --prefix build/native-stage --component macos-ime
build/native-stage/bin/vninput-input-source --disable
# Lệnh trên chuyển sang bàn phím ASCII nếu VNInput đang được chọn.
# Dừng đúng tiến trình VNInput đang chạy trước khi thay bundle.
python3 scripts/install-macos.py --stage build/native-stage --input-method-only
```

Installer sao lưu bundle cũ vào `~/Library/Application Support/VNInput/Backups/`,
đăng ký và bật nguồn nhập mới, không tự chọn VNInput. `--input-method-only` không
thay ứng dụng Settings. Chọn VNInput trong menu nguồn nhập hoặc chạy
`build/native-stage/bin/vninput-input-source --select` để dùng bản đã cài.
`--select-ascii` chuyển về bàn phím ASCII; `--disable` đồng thời tắt VNInput.

## Gạch chân composition

Ngày 2026-10-03: Với editor trả về vị trí chọn và đoạn chữ đã nhập, adapter
chèn chữ dưới dạng text thường và thay đúng đoạn chữ của từ hiện tại bằng
`insertText:replacementRange:`. Nó kiểm tra lại vị trí lẫn nội dung trước mỗi
lần thay; nếu editor, Undo hay con trỏ làm chữ thay đổi, adapter bỏ quyền sở
hữu đoạn đó và bắt đầu từ mới. Không dùng Backspace giả lập. Với editor không
truy cập được tài liệu, adapter vẫn dùng `setMarkedText:` như trước; gạch chân
có thể còn xuất hiện ở các editor ấy.

Ngày 2026-11-11: KetPlus Terminal không còn ngoại lệ theo bundle ID. Từ bản có
ketvietlab/ketplus#532, `TerminalView` báo phần chữ gõ từ lần cuối con trỏ shell
di chuyển (mũi tên, Tab, Enter, click, dán) làm chữ trước con trỏ, và đổi một lần
`insertText:replacementRange:` thành Backspace rồi chữ mới gửi xuống PTY. Vì vậy
adapter dùng luồng thay chữ như với editor: zsh thấy từng chữ ngay khi gõ, gợi ý
và Tab hoàn thành đúng. Bản KetPlus Terminal cũ hơn chưa thay được chữ đã gửi;
hãy cập nhật terminal trước khi dùng bản VNInput này.

Kiểm thử mock IMK bao gồm Telex/VNI, Backspace, Esc, selection, đổi client,
đổi con trỏ, thay nội dung bên ngoài và fallback. Bài kiểm tra `vninput-qt-bridge`
chạy với QNSView thật trên desktop: Telex/VNI ra `tiếng`, không phát preedit hay
gọi `setMarkedText:`. Đây là kiểm tra giao tiếp Qt, chưa chứng minh Chrome qua
thao tác IME end-to-end. Mỗi lần sửa trực tiếp có thể thành một bước Undo riêng
tuỳ editor; cần xác nhận trải nghiệm Undo trong ứng dụng đích.

Bản cũ gửi `NSUnderlineStyleNone` cùng màu trong suốt nhưng Chrome vẫn vẽ nét
liền trong composition: Chromium chuyển style 0 thành nét mảnh; Blink coi màu
transparent là dùng màu chữ. Nếu bỏ underline attributes, Blink lại tự thêm
gạch chân mặc định. Vì vậy cách sửa hiện tại tránh composition khi có thể.

Nguồn đối chiếu (nhánh main, đọc ngày 2026-10-02):
- [Chromium ExtractUnderlines](https://github.com/chromium/chromium/blob/main/content/app_shim_remote_cocoa/render_widget_host_view_cocoa.mm).
- [Blink UseTextColor](https://github.com/chromium/chromium/blob/main/third_party/blink/renderer/core/editing/markers/styleable_marker.cc).
- [Blink fallback composition](https://github.com/chromium/chromium/blob/main/third_party/blink/renderer/core/editing/ime/input_method_controller.cc).

Chưa xác minh trực tiếp Chrome bằng thao tác IME end-to-end trong lượt này.

## Icon menu nguồn nhập

`tsInputMethodIconFileKey` trỏ đến `vninput-input-source.pdf`, một biểu tượng V dạng
vector có MediaBox **16 × 16 point**. `TISIconIsTemplate=true` cho phép hệ thống
tô màu theo giao diện. Tái tạo asset bằng
`python3 scripts/generate-input-source-icon.py`; không cần thư viện ngoài.
Logo ứng dụng `vninput-logo.png` vẫn được dùng cho `CFBundleIconFile` và Qt.

Lỗi ngày 2026-09-30: menu nguồn nhập từng dùng trực tiếp logo **1254 × 1254 px
ở 72 DPI**. AppKit đọc thành **1254 × 1254 point**, làm dòng VNInput kéo dài hơn
màn hình. Kích thước logic của ảnh menu phải được kiểm tra độc lập với logo app.

`ctest --preset dev` chạy kiểm tra `macos_bundle` trên bundle vừa build: nạp icon
bằng AppKit, giới hạn mỗi chiều tối đa 18 point và kiểm tra icon không làm menu
native cao thêm. Với bản sửa, menu một dòng cao 34 point cả khi có và không có
icon. Bản cũ bị bài kiểm tra từ chối ở kích thước 1254 point.

Sau cài đặt, `vninput-input-source --status` đọc đường dẫn icon và kích thước point
từ đăng ký TIS của macOS, không chỉ từ file build. Lần sửa này đã xác minh nguồn
nhập đã cài dùng icon mới 16 × 16 point, chữ ký hợp lệ và 5/5 bài kiểm tra đạt.
Đây là kết quả trước khi đổi thương hiệu. Ngày 2026-10-01, mã nguồn, bundle ID
và đường dẫn cấu hình đã đổi sang VNInput; build sạch và 5/5 kiểm thử đạt với
tên mới. Bản cài đang chạy trên máy không được thay thế trong bước đổi tên mã
nguồn. Bộ cài mới sử dụng `~/Library/Input Methods/VNInput.app` và cấu hình
`~/Library/Preferences/VNInput/VNInput/settings.json`.

System Settings đang mở có thể tiếp tục giữ icon cũ trong bộ nhớ, ngay cả khi
TIS đã trỏ đến asset mới. Ngày 2026-09-30 đã tái hiện dòng VNInput quá cao trong
Keyboard → Input Sources, thoát hẳn System Settings bằng Cmd+Q rồi mở lại và
kiểm tra trực tiếp: icon V, tên VNInput và vùng chọn màu xanh đều gọn trong một
dòng bình thường. Chỉ đóng/mở hộp thoại Input Sources chưa chắc xoá cache này.
Không cần cài lại hay đăng xuất cho trường hợp đã kiểm chứng này.

Tham khảo cấu hình bundle:
[Apple InputMethodKit](https://developer.apple.com/documentation/inputmethodkit/imkserver/init%28name%3Abundleidentifier%3A%29).
