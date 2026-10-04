# VNInput · Compact desktop mockup

Màn hình **640 × 480 px**, menu **272 × 302 px**. Nội dung tiếng Việt.
Theo yêu cầu người dùng: UI tự do, chỉ dùng token màu Két DS.

Xem [cách chạy, nguồn thiết kế và giới hạn](../README.md).

Từ thư mục gốc workspace:

```sh
npm ci --prefix design
npx --yes ketatlas@0.5.2 serve ./design/vninput.ketatlas --renderer --port 61740 --html-port 61741
npx --yes ketatlas@0.5.2 audit ./design/vninput.ketatlas --strict
```

## Phạm vi

| Luồng | Screen ID | Trạng thái và tương tác |
|---|---|---|
| Chọn kiểu gõ | settings, playground | Telex/VNI, vị trí dấu, checkbox, lưu; xác nhận/hủy khôi phục |
| Đổi phím chuyển | shortcuts | Ghi tổ hợp, gợi ý, xung đột Ctrl + Space, chọn lại và lưu |
| Kích hoạt hệ thống | integration, playground | macOS/Windows/Linux, chưa kích hoạt/sẵn sàng, mở Gõ thử |
| Menu V/E | quick-menu, settings, about | Việt/Anh, Telex/VNI, mở thiết lập và giới thiệu |
| Thử nội dung | playground | Trống, câu mẫu, nhập tự do, đếm ký tự, xóa |

Tất cả screen/node URL render trực tiếp, không cần thao tác trước.
View model ở `../src/fixtures.ts`, presenter ở `../src/presenters.tsx`.
Query phổ biến: platform=macos|windows|linux, method=telex|vni, enabled=0|1,
state=restore|restored|conflict|ready|sample, saved=1.

## Renderer

- Két DS 0.1.29, chỉ dùng semantic color tokens; nguồn và MIT license ở README cha.
- KetJS + KetJS View 0.1.29, server-rendered.
- Adapter: token-only native integration. Descriptor đã đọc; static materializer
  không áp dụng và không có materializer lock.
- Package-lock cố định dependency ngoài bundle.
- Light/Dark dùng semantic colors; theme bridge không dựng lại form.

`atlas.progress.json` theo dõi sản phẩm native. Các màn đã được xác định
phạm vi nhưng chưa triển khai Qt/native; không dùng trạng thái Verified cho mock.

## Kết quả review mockup

Ngày 2026-09-30: typecheck, validate, strict audit pass. 22 screen/state URLs
và asset trả HTTP 200. Đã kiểm tra giao diện, các tương tác chính, đường khôi
phục và theme synchronization trong browser. Xem
[báo cáo và giới hạn kiểm tra](../evidence/verification.json).
