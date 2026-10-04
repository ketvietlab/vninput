# Engine VNInput

`core/src/engine.cpp` là implementation C++20 độc lập, không phụ thuộc Qt hay
engine bộ gõ bên ngoài. Không tích hợp, liên kết hoặc sao chép mã OpenKey.
Quy ước Telex/VNI được đối chiếu với
[hướng dẫn UniKey](https://www.unikey.org/support/ukmanual.html); không sử dụng mã
nguồn UniKey. Bộ kiểm thử mô tả hành vi riêng của VNInput, không tuyên bố tương
thích hoàn toàn với bất kỳ bộ gõ nào khác.

## Đã hỗ trợ

| Quy tắc | Telex | VNI |
| --- | --- | --- |
| Sắc, huyền, hỏi, ngã, nặng | `s f r x j` | `1 2 3 4 5` |
| Xoá dấu thanh, giữ dấu nguyên âm | `z` | `0` |
| â, ê, ô | `aa ee oo` | `a6 e6 o6` |
| ă, ơ, ư, đ | `aw ow uw dd` | `a8 o7 u7 d9` |
| Phím lặp trả về chữ thường | `ass → as`, `aaa → aa` | `e11 → e1`, `a66 → a6` |

- Xuất nguyên âm tiếng Việt dựng sẵn Unicode NFC, giữ hoa/thường của chữ gốc.
- Chọn cụm nguyên âm cuối, loại phụ âm lướt trong `qu`/`gi`; vẫn đặt dấu vào
  `i` trong `gì`, `gìn`. Dấu di chuyển khi cụm nguyên âm hoặc phụ âm cuối thay đổi.
- Modern: `hoà`, `hoè`, `thuỷ`; Classic: `hòa`, `hòe`, `thủy`.
- Kết hợp `ươ`, nhập dấu trước/sau phụ âm cuối khi `freeTone=true`.
  Có thể gõ `dduwowngf`, `dduowngf`, `duongwwfd`, `d9u7o7ng2`, `duong9772`.
  `thuở`, `huơ`, `quơ` giữ `u`; thêm phần cuối thích hợp cho ra `thương`, `hươu`.
- `freeTone=false` yêu cầu dấu ngay sau cụm nguyên âm, `dd/d9` ngay sau `d`.
- `w → ư`, `ww → w`; `WWindowws → Windows`, `guitarr → guitar`.
- Backspace xoá một **ký tự đang hiển thị**, không phải một phím vật lý.
  `tiếng → tiến → tiế → ti`. Dấu trên chữ bị xoá không sống lại khi Esc.
- Esc khôi phục chuỗi phím còn thuộc từ hiện tại rồi giữ từ đó ở chế độ literal.
  Esc lần hai commit và chuyển tiếp cho ứng dụng. Tắt `restoreOnEscape` thì Esc
  commit chữ đã ghép và chuyển tiếp ngay.
- Phím lặp thoát dấu cũng giữ phần còn lại của từ ở chế độ literal, trừ `ww`
  độc lập và `ooo` (để tiếp tục gõ `Windows`, `xoòng`). Trạng thái này kết thúc
  ở ranh giới từ/reset, không vô hiệu hoá engine cho những từ tiếp theo.

## Contract và vòng đời

```cpp
auto engine = vninput::makeEngine(settings); // một instance / input context
auto result = engine->process({vninput::Key::Text, U"a"});
// Host: áp dụng result.commit, thay preedit bằng result.preedit;
// nếu !result.consumed, tiếp tục xử lý event gốc đúng một lần.
```

`KeyEvent::text` là ký tự sau khi áp dụng layout và Shift/Caps Lock, dùng UTF-32.
Shift không tự đổi case; adapter gửi đúng case. Ctrl/Alt/Meta commit rồi chuyển
tiếp để ứng dụng xử lý tổ hợp phím. Key release thuộc host, không gửi vào engine.

Space, Enter, dấu câu, điều hướng, Unicode đã được IME khác xử lý và một event
nhiều ký tự đều commit rồi đi qua nguyên vẹn. Không đưa văn bản paste hoặc commit
từ OS IME trở lại chuỗi phím Telex. `flush()` commit một lần và xoá trạng thái;
`reset()` huỷ composition, không commit. Host phải tự áp dụng preedit rỗng khi
huỷ. Không dùng `reset()` thay cho `flush()` nếu cần giữ chữ người dùng vừa gõ.

`configure()` kiểm tra enum, hoãn đổi các lựa chọn engine tới lần `process()`
hoặc `flush()` kế tiếp để không mất từ đang nhập. Lần đó commit bằng quy tắc cũ
rồi dùng cấu hình mới. Đổi riêng phím tắt không ảnh hưởng core. Cấu hình invalid
ném `std::invalid_argument` và không thay đổi trạng thái.

Mỗi instance chỉ được gọi tuần tự; nhiều context có thể dùng nhiều instance
độc lập. Không hook phím, không ghi nội dung, không file I/O, không network.
Giới hạn 64 chữ đang ghép hoặc 256 phím lịch sử; khi vượt giới hạn, commit phần
trước rồi bắt đầu composition mới, không làm mất đầu vào. Esc có thể mở rộng
preedit tới tối đa 256 ký tự phím gốc.

## Giới hạn hiện tại

- Có Gõ thử và adapter InputMethodKit thử nghiệm; TSF/Fcitx5 chưa triển khai.
  Kiểm thử core và mock client không chứng minh được độ ổn định khi đổi app/editor
  thật. Xem trạng thái xác minh tại [macos.md](macos.md).
- Chưa có từ điển, kiểm tra chính tả hay tự hoàn nguyên từ tiếng Anh. Dùng Esc
  hoặc chuyển E khi cần giữ nguyên từ. Không cung cấp macro, gõ tắt hay bảng mã
  legacy (TCVN3/VNI Windows).
- Không chỉnh lại từ đã commit khi Backspace vượt ranh giới; lúc đó host xoá
  văn bản. Không nhận diện dấu tách rời/NFD để tái ghép; Unicode ngoài phím gõ
  ASCII được chuyển tiếp. `[`/`]` giữ chức năng dấu ngoặc.
- Vòng đời adapter native, secure input, IME cạnh tranh và hotkey phải được
  kiểm thử trên từng OS. Không hứa loại trừ hoàn toàn lỗi mất khả năng gõ chỉ từ
  kết quả kiểm thử thuật toán.

## Kiểm thử

`ctest --preset core-only` chạy core không có Qt. `ctest --preset dev` thêm
kiểm thử Qt: nhập/preedit/commit, bộ đếm, chuyển Telex/VNI/V/E, Backspace/Esc,
paste, selection, clear/setPlainText, undo/redo, con trỏ, focus và OS IME.

Engine có các ca từ/câu, toàn bộ nguyên âm × dấu × case, trạng thái độc lập,
10.000 ký tự liên tiếp, 50.000 event ngẫu nhiên có seed cố định, 2.000 chu kỳ
thoát dấu rồi gõ từ mới, và giới hạn lịch sử khi đổi dấu liên tục. Các kiểm thử
dài kiểm tra phục hồi trạng thái/bộ nhớ, không thay thế kiểm thử IME native.
