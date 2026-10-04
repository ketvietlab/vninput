# Kiểm tra độ khớp Qt / KetAtlas

Ngưỡng hiện tại **3%**, được người dùng cho phép sau khi yêu cầu ban đầu là 2%.
Không thay đổi baseline gốc để giảm sai khác. Không crop hay mask vùng nào.

1. Build preset `dev`, rồi chụp widget thật:

   ```sh
   QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=1 \
     ./build/dev/tests/vninput-ui-capture design/evidence/qt/candidate
   ```

2. Mở mock gốc bằng KetAtlas theo `design/README.md`. Dùng skill `ketplus-browser`
   và các MCP `browser_*` để chụp 5 trang chính, `settings?state=restore` và
   `quick-menu`. Giữ light, DPR 1, locale en-US; viewport 640 × 480 hoặc 272 × 302
   cho menu. Lưu PNG nguyên vẹn vào `design/evidence/qt/reference`.
3. Chạy `python3 tests/visual/serve.py`. Server chỉ phục vụ ảnh trong thư mục
   evidence qua loopback 127.0.0.1:61742, không phục vụ source hay cấu hình người dùng.
4. Dùng hai trang trong KetPlus có môi trường giống nhau:
   `http://127.0.0.1:61742/?side=reference&page=settings` và
   `http://127.0.0.1:61742/?side=candidate&page=settings`.
5. Với mỗi bên, gọi `browser_observe` kèm `includeScreenshot: true`, rồi
   `visual_compare` với đúng cặp `snapshotId`/`captureId` và policy:

   ```json
   { "maxPixelMismatchRatio": 0.03, "pixelThreshold": 0.1, "maxFindings": 5 }
   ```

6. Lặp cho `shortcuts`, `playground`, `integration`, `about`, `restore`,
   `quick-menu`; lưu kết quả/ảnh diff trước khi đóng mọi session KetPlus đã tạo.
   Dừng server ảnh sau khi kiểm tra.

Chỉ gọi kết quả này là raster comparison. Các HTML shell cùng chứa một ảnh, nên
semantic matching của shell không xác nhận widget native. Kiểm tra hành vi thật
bằng `ctest --preset dev`. Thay đổi màu/font/DPI, hệ điều hành hoặc Qt version cần
được đo lại; không ngoại suy kết quả macOS light sang các môi trường đó.
