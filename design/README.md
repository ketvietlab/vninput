# VNInput — thiết kế giao diện

Mockup desktop **640 × 480 px**; menu V/E **272 × 302 px**.
Bundle chỉnh sửa được: [vninput.ketatlas](./vninput.ketatlas/atlas.json).

## Chạy

Node.js **24+** (yêu cầu của KetJS). Từ thư mục gốc workspace:

```sh
npm ci --prefix design
npx --yes ketatlas@0.5.2 serve ./design/vninput.ketatlas --renderer --port 61740 --html-port 61741
```

- Atlas: <http://127.0.0.1:61740>
- Màn hình trực tiếp: <http://127.0.0.1:61741/__atlas/vninput/settings>
- Bấm **Open screen** trên node để tương tác trong Atlas.

## Nguồn thiết kế

Theo yêu cầu cập nhật: **chỉ dùng token màu của Két Design System**. Markup,
layout, typography, spacing, icon và hành vi là thiết kế riêng VNInput.
Không tuyên bố tuân thủ component/layout của Két DS.

- Nguồn: https://github.com/ketvietlab/ketjs/tree/develop/packages/design-system
- Package cố định: `@ketvietlab/design-system@0.1.29`, giấy phép MIT.
- Chỉ tải stylesheet `foundations/tokens.css` trực tiếp từ package đã cài.
  CSS VNInput chỉ tham chiếu các biến màu; không tải component CSS/runtime.
- Đã đọc descriptor `@ketvietlab/design-system/atlas/profile.json`.
  Static materializer không áp dụng cho tích hợp token-only theo yêu cầu.
- Phiên bản và integrity được cố định trong `package-lock.json`;
  không có materializer lock cho tích hợp này.

## Cấu trúc

```text
design/
  src/fixtures.ts          View model từ URL
  src/presenters.tsx       Presenter KetJS, markup riêng VNInput
  src/server.tsx           KetJS server và route Atlas
  public/vninput.css         Layout compact, token màu Két
  public/interactions.js   Nhập thử, đếm ký tự, ghi phím
  public/theme.js          Cầu nối Light/Dark với Atlas
  public/brand/            Logo PNG trong suốt và prompt tạo ảnh
  vninput.ketatlas/          Flow graph, renderer contract, progress
  evidence/               Ảnh kiểm tra và báo cáo
```

HTML render bằng KetJS server components. Viewer và renderer dùng hai cổng
loopback riêng; không có package installation nằm trong bundle.

Logo được tạo bằng built-in image_gen và đã đưa vào mockup:
[PNG gốc](./public/brand/vninput-logo.png), [prompt](./public/brand/README.md).
UI dùng alpha của PNG làm mask và tô bằng token accent.

## Phạm vi và giới hạn

Mockup để triển khai Qt/C++ sau. Không đăng ký IME, không gõ toàn hệ thống,
không ghi thiết lập OS, không dò xung đột phím tắt thật.
Ô gõ thử nhận bàn phím hiện tại và có câu mẫu; chưa có engine Telex/VNI.
Trạng thái xung đột phím tắt là fixture minh hoạ.
“Tôi đã chọn VNInput” chỉ chuyển fixture sang trạng thái sẵn sàng.

Thiết lập mô phỏng nằm trong URL; văn bản gõ thử chỉ ở DOM hiện tại.
Nhãn phím theo `platform`: macOS dùng `⌥`/`⌃`/`⇧`/`⌘`; Windows/Linux dùng tên
Ctrl/Alt/Shift. Giá trị gửi trong URL vẫn là Qt PortableText. Menu nhanh hiển thị
phím đã chọn. Cả presenter và phần ghi phím dùng formatter chung trong
`public/shortcuts.js`. [Báo cáo sửa nhãn macOS](./evidence/mac-shortcuts/comparison.json).
Menu V/E mô phỏng nội dung chung, hình thức native phụ thuộc OS.
Link từ menu sang Settings vẫn ở kích thước iframe menu; mở node Settings
riêng để xem đúng 640 × 480.

Tiến độ trong Atlas theo dõi triển khai Qt/native. Có mockup không đồng nghĩa
với sản phẩm đã được Implemented/Verified.

## Kiểm tra

```sh
npm run check --prefix design
npx --yes ketatlas@0.5.2 discover ./design --json
npx --yes ketatlas@0.5.2 validate ./design/vninput.ketatlas
npx --yes ketatlas@0.5.2 audit ./design/vninput.ketatlas --strict
```

Kết quả ngày 2026-09-30: typecheck, validate và strict audit đều pass;
discovery tìm thấy 1 bundle, không lỗi. 22 URL màn hình/trạng thái và các
asset cần thiết trả HTTP 200. Đã kiểm tra tương tác trong Chromium qua KetPlus.

Xem [báo cáo kiểm tra](./evidence/verification.json),
[Bộ gõ 640 × 480](./evidence/settings-light.png),
[menu V/E](./evidence/quick-menu.png),
[preview tối giữ nguyên nội dung](./evidence/atlas-dark-preserved-input.png)
và [tắt sync khôi phục nền sáng](./evidence/atlas-sync-off.png).
