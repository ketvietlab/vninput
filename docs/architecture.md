# Kiến trúc VNInput

## Ranh giới

`VNInput::Core` chỉ dùng thư viện chuẩn C++20. `Settings` giữ cấu hình cùng enum ổn
định. `makeEngine()` tạo engine Telex/VNI riêng cho từng input context, xuất
Unicode NFC. Không có trạng thái nhập dùng chung giữa các instance.

`Engine::process` nhận phím đã được adapter chuẩn hoá. Kết quả quy định rõ event có
bị tiêu thụ không, chuỗi commit và toàn bộ preedit mới. Adapter chịu trách nhiệm
focus/selection, chuyển key event, cập nhật preedit và commit qua API hệ điều hành.
Adapter phải áp dụng commit/preedit **trước** khi chuyển tiếp event chưa được
tiêu thụ. Khi có adapter native, không gửi nội dung gõ qua tiến trình Qt Settings.
Hiện `EngineTextEdit` là adapter cục bộ cho ô Gõ thử, dùng `QInputMethodEvent`
để preedit không làm bẩn lịch sử undo. Focus, di chuyển con trỏ, paste và thay
đổi cấu hình kết thúc composition; văn bản từ IME hệ điều hành đi qua nguyên vẹn.
Quy tắc, vòng đời, test và giới hạn engine: [engine.md](engine.md).

`VNInput::Platform` cung cấp tên OS/API; `implemented=true` trên macOS chỉ khi build
kèm adapter InputMethodKit. Adapter Objective-C++ liên kết core và framework hệ
thống, không liên kết Qt. TSF trên Windows và Fcitx 5 trên Linux vẫn là scaffold.
Trạng thái thử nghiệm và kiểm tra bundle: [macos.md](macos.md).

`vninput-settings` là tiến trình Qt chạy khi mở cài đặt. UI không đăng ký hook bàn
phím hay chạy tray nền. Trên macOS, Qt và adapter dùng chung JSON tại
`~/Library/Preferences/VNInput/VNInput/settings.json`; Qt phát distributed notification
sau khi lưu. Adapter đọc bằng Foundation và giữ cấu hình hợp lệ gần nhất nếu
file hỏng; mỗi input context có engine riêng.

## Giao diện

Presenter native nằm trong `SettingsWindow`, các control có hình thức riêng nằm
trong `controls.hpp`, token màu trong `theme.cpp`. Widget nhập vẫn là Qt
`QTextEdit`, `QKeySequenceEdit`, `QComboBox`, checkbox và button; không dùng ảnh
mock làm nền hay điều khiển giả. Những lớp paint nhỏ giữ hình học gần CSS mock.

Số đo baseline lấy từ `design/public/vninput.css` và presenter KetJS ở kích thước
640 × 480. Giao diện native không tải HTML/CSS hoặc cần server khi chạy.
Font hệ thống macOS được đọc từ OS, không đóng gói font Apple. Nền tảng khác dùng
font hệ thống của Qt; cần nghiệm thu hình ảnh riêng trên từng OS/DPI.

## Cấu hình v1

`schemaVersion`, `inputMethod` (`telex|vni`), `tonePlacement` (`modern|classic`),
`enabled`, `freeTone`, `restoreOnEscape`, `toggleShortcut` (Qt PortableText).
Version không hỗ trợ, giá trị enum lạ, sai kiểu hay phím tắt thiếu modifier đều
bị từ chối. Writer dùng `QSaveFile` với cơ chế commit, không mở đường ghi trực
tiếp thay thế. Văn bản gõ thử không xuất hiện trong schema.

Chưa giải quyết đa tiến trình chỉnh cùng cấu hình hoặc IPC. Khi làm adapter,
quy định một bên sở hữu writer hoặc bổ sung locking/versioning và migration.

## Trình tự phát triển tiếp

1. Adapter native đầu tiên cùng vòng đời input context; kiểm tra app đích,
   Unicode, focus, paste, tổ hợp phím và các editor khó.
2. Cấu hình dùng chung, bật/tắt thật, menu native và phím tắt toàn hệ thống.
3. Adapter hai OS còn lại, build/test trên host thật.
4. Đóng gói, icon bundle, signing, notarization và phân phối theo nền tảng.

Scaffold được kiểm tra trên macOS arm64. Cấu trúc và nhánh CMake Windows/Linux
được chuẩn bị, chưa có bằng chứng build hay tích hợp IME trên hai nền tảng đó.
