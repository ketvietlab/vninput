import { formatShortcut } from './shortcuts.js';
const root = document.getElementById('vninput-root');
const platform = root?.dataset.platform || 'macos';
const sample = document.getElementById('typing-pad');
const count = document.getElementById('character-count');
function updateCount() {
  if (sample && count) count.textContent = Array.from(sample.value).length + ' ký tự';
}
sample?.addEventListener('input', updateCount);
document.querySelector('[name="insert-sample"]')?.addEventListener('click', () => {
  sample.value = 'Tiếng Việt thật đẹp.\nGõ tự nhiên, viết điều bạn muốn.';
  updateCount();
  sample.focus();
});
document.querySelector('[name="clear-sample"]')?.addEventListener('click', () => {
  sample.value = '';
  updateCount();
  sample.focus();
});
document.querySelectorAll('[role="switch"]').forEach(input => input.addEventListener('change', () => {
  input.setAttribute('aria-checked', String(input.checked));
}));
document.querySelector('[data-settings-form]')?.addEventListener('change', () => {
  const hint = document.querySelector('[data-save-hint]');
  if (hint) hint.textContent = 'Có thay đổi chưa lưu';
});
document.querySelectorAll('[name="method"]').forEach(input => input.addEventListener('change', () => {
  const equation = document.querySelector('.typing-equation code');
  const help = document.getElementById('method-help');
  if (equation) equation.textContent = input.value === 'vni' ? 'tie6ng1 Vie6t5' : 'tieengs Vieetj';
  if (help) help.textContent = input.value === 'vni' ? 'Dùng số để thêm dấu.' : 'Dùng chữ để thêm dấu.';
}));
const capture = document.querySelector('[name="capture-shortcut"]');
const shortcut = document.getElementById('shortcut');
const captureStatus = document.getElementById('capture-status');
let recording = false;
capture?.addEventListener('click', () => {
  recording = true;
  captureStatus.textContent = 'Nhấn tổ hợp mới… Esc để huỷ.';
  shortcut.focus();
});
shortcut?.addEventListener('keydown', event => {
  if (!recording) return;
  event.preventDefault();
  if (event.key === 'Escape') {
    recording = false;
    captureStatus.textContent = 'Đã huỷ ghi phím tắt.';
    capture.focus();
    return;
  }
  if (['Control', 'Alt', 'Shift', 'Meta'].includes(event.key)) return;
  if (!event.ctrlKey && !event.altKey && !event.metaKey) {
    captureStatus.textContent = platform === 'macos' ? 'Thêm ⌃ Control, ⌥ Option hoặc ⌘ Command vào tổ hợp.' : 'Thêm Ctrl, Alt hoặc Meta vào tổ hợp.';
    return;
  }
  const portable = [
    event.ctrlKey && (platform === 'macos' ? 'Meta' : 'Ctrl'), event.altKey && 'Alt',
    event.shiftKey && 'Shift', event.metaKey && (platform === 'macos' ? 'Ctrl' : 'Meta'),
    event.code === 'Space' ? 'Space' : event.code.startsWith('Key') ? event.code.slice(3) : event.key.toUpperCase(),
  ].filter(Boolean).join(' + ');
  document.querySelector('[name="shortcut"]').value = portable;
  shortcut.value = formatShortcut(portable, platform);
  recording = false;
  captureStatus.textContent = 'Đã ghi tổ hợp. Bấm Lưu thay đổi để áp dụng.';
  shortcut.dispatchEvent(new Event('change', { bubbles: true }));
});
