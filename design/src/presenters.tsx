import type { JSXChild } from '@ketvietlab/ketjs-view'
import { platformNames, route, type ScreenId, type ViewModel } from './fixtures.ts'
import { formatShortcut } from '../public/shortcuts.js'

const paths: Record<string, JSXChild> = {
  settings: <><rect x="3" y="5" width="18" height="14" rx="3"/><path d="M7 9h.01M11 9h.01M15 9h.01M19 9h.01M7 13h.01M11 13h.01M15 13h.01M7 16h10"/></>,
  shortcuts: <path d="m13 3-9 11h7l-1 7 10-12h-7z"/>,
  playground: <><path d="m4 20 4-1 12-12-3-3L5 16zM14 7l3 3"/></>,
  integration: <><rect x="3" y="3" width="18" height="13" rx="2"/><path d="M8 21h8M12 16v5m-3-11 2 2 4-4"/></>,
  about: <><circle cx="12" cy="12" r="9"/><path d="M12 11v6M12 7h.01"/></>,
}
const Icon = ({ name }: { name: string }) => <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.65" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">{paths[name]}</svg>
const Brand = () => <span class="brand"><span class="brand-mark" aria-hidden="true"/><strong>VNInput</strong></span>
type ActionProps = { label: string; href?: string; primary?: boolean; quiet?: boolean; name?: string; value?: string; type?: 'button' | 'submit' }
const Action = (p: ActionProps) => p.href ? <a class={'button' + (p.primary ? ' primary' : '') + (p.quiet ? ' quiet' : '')} href={p.href}>{p.label}</a> : <button class={'button' + (p.primary ? ' primary' : '') + (p.quiet ? ' quiet' : '')} type={p.type || 'button'} name={p.name} value={p.value}>{p.label}</button>
const Hidden = ({ vm }: { vm: ViewModel }) => <><input type="hidden" name="platform" value={vm.platform}/><input type="hidden" name="enabled" value={vm.enabled ? '1' : '0'}/></>
const Feedback = ({ vm }: { vm: ViewModel }) => <p class={'form-feedback' + ((vm.saved && !vm.conflict) || vm.restored ? ' success' : '')} role="status" data-save-hint="">{vm.conflict ? 'Chọn một tổ hợp khác' : vm.saved ? 'Đã lưu thay đổi' : vm.restored ? 'Đã khôi phục mặc định' : 'Áp dụng từ lần gõ tiếp theo'}</p>
const FormFooter = ({ vm }: { vm: ViewModel }) => <footer class="form-footer"><Feedback vm={vm}/><div class="actions"><Action label="Mặc định" quiet href={route(vm, vm.screen, { state: 'restore' })}/><Action label="Lưu thay đổi" primary type="submit" name="saved" value="1"/></div></footer>
const Check = ({ name, title, help, checked }: { name: string; title: string; help: string; checked: boolean }) => <label class="check-row"><input type="checkbox" name={name} value="1" checked={checked}/><span><strong>{title}</strong><small>{help}</small></span></label>

function Settings({ vm }: { vm: ViewModel }) {
  return <form class="page-form" action="/__atlas/vninput/settings" method="get" data-settings-form=""><Hidden vm={vm}/><input type="hidden" name="shortcut" value={vm.shortcut}/>
    <div class="form-body">
      <div class="setting-row"><span class="field-label" id="method-label">Kiểu gõ</span><div><div class="segmented" role="radiogroup" aria-labelledby="method-label">{(['telex', 'vni'] as const).map(method => <label><input type="radio" name="method" value={method} checked={vm.method === method}/><span>{method === 'telex' ? 'Telex' : 'VNI'}</span></label>)}</div><p class="field-note" id="method-help">{vm.method === 'telex' ? 'Dùng chữ để thêm dấu.' : 'Dùng số để thêm dấu.'}</p></div></div>
      <div class="setting-row example-row"><span class="field-label">Ví dụ</span><div class="typing-equation"><code>{vm.method === 'telex' ? 'tieengs Vieetj' : 'tie6ng1 Vie6t5'}</code><span class="arrow">→</span><strong>tiếng Việt</strong></div></div>
      <div class="setting-row"><label class="field-label" for="tone">Vị trí dấu</label><select id="tone" name="tone"><option value="modern" selected={vm.tone === 'modern'}>Kiểu mới · hoà, thuý</option><option value="classic" selected={vm.tone === 'classic'}>Kiểu cũ · hòa, thúy</option></select></div>
      <div class="form-divider"/>
      <Check name="free-tone" title="Bỏ dấu tự do" help="Đặt dấu trước hoặc sau phụ âm cuối." checked={vm.modern}/>
      <Check name="restore-word" title="Khôi phục từ gốc bằng Esc" help="Trả lại chuỗi phím khi từ vẫn đang soạn." checked={vm.restoreWord}/>
      <p class="encoding-note">Bảng mã <strong>Unicode dựng sẵn</strong></p>
    </div><FormFooter vm={vm}/>
  </form>
}

function Shortcuts({ vm }: { vm: ViewModel }) {
  return <form class="page-form" action="/__atlas/vninput/shortcuts" method="get" data-settings-form=""><Hidden vm={vm}/><input type="hidden" name="method" value={vm.method}/><input type="hidden" name="tone" value={vm.tone}/><input type="hidden" name="free-tone" value={vm.modern ? '1' : '0'}/><input type="hidden" name="restore-word" value={vm.restoreWord ? '1' : '0'}/>
    <div class="form-body">
      <div class="shortcut-summary"><span class="letter-tile">V</span><span class="arrow">⇄</span><span class="letter-tile secondary-tile">E</span><span>Chuyển nhanh Việt / Anh</span></div>
      <label class="field-label" for="shortcut">Phím chuyển</label><div class="shortcut-field"><input type="hidden" name="shortcut" value={vm.shortcut}/><input id="shortcut" value={formatShortcut(vm.shortcut, vm.platform)} readOnly aria-invalid={vm.conflict ? 'true' : undefined} aria-describedby="capture-status"/><Action label="Ghi phím tắt" name="capture-shortcut"/></div>
      <p class="field-note" id="capture-status" role="status">Nhấn Ghi phím tắt, rồi bấm tổ hợp mới.</p>
      {vm.conflict && <div class="notice warning" role="alert"><strong>Phím tắt đang được sử dụng</strong><span>{formatShortcut('Ctrl + Space', vm.platform)} trùng phím hệ thống. Chọn tổ hợp khác.</span></div>}
      <div class="suggestions"><span>Gợi ý</span><a href={route(vm, 'shortcuts', { shortcut: 'Alt + Z' })}><kbd>{formatShortcut('Alt + Z', vm.platform)}</kbd></a><a href={route(vm, 'shortcuts', { shortcut: 'Ctrl + Shift + V' })}><kbd>{formatShortcut('Ctrl + Shift + V', vm.platform)}</kbd></a></div>
      <p class="field-note">{vm.platform === 'macos' ? '⌥ Option · ⌃ Control · ⇧ Shift · ⌘ Command' : 'Ctrl, Alt và Shift là các phím bổ trợ.'}</p>
      <details class="more-shortcuts" open={vm.conflict}><summary>Phím hệ thống cần lưu ý</summary><a href={route(vm, 'shortcuts', { shortcut: 'Ctrl + Space', state: 'conflict' })}>{formatShortcut('Ctrl + Space', vm.platform)}</a><span> · Có thể trùng phím tắt hệ thống.</span></details>
    </div><FormFooter vm={vm}/>
  </form>
}

function Playground({ vm }: { vm: ViewModel }) {
  const sample = vm.sample ? 'Tiếng Việt thật đẹp.\nGõ tự nhiên, viết điều bạn muốn.' : ''
  const pairs = vm.method === 'telex' ? [['aa','â'],['aw','ă'],['ee','ê'],['oo','ô'],['ow','ơ'],['uw','ư'],['dd','đ']] : [['a6','â'],['a8','ă'],['e6','ê'],['o6','ô'],['o7','ơ'],['u7','ư'],['d9','đ']]
  return <div class="page-form"><div class="form-body">
    <div class="editor-heading"><label class="field-label" for="typing-pad">Viết vài dòng của bạn</label><span class="mini-label">{vm.method === 'telex' ? 'Telex' : 'VNI'}</span></div>
    <textarea id="typing-pad" name="typing-pad" placeholder="Bắt đầu gõ ở đây…" spellcheck="false">{sample}</textarea>
    <div class="editor-toolbar"><span id="character-count" aria-live="polite">{Array.from(sample).length} ký tự</span><div class="actions"><Action label="Điền câu mẫu" name="insert-sample" quiet/><Action label="Xoá" name="clear-sample" quiet/></div></div>
    <div class="reference-grid">{pairs.map(([key,value]) => <div><code>{key}</code><span>{value}</span></div>)}</div>
    <p class="tone-reference"><code>{vm.method === 'telex' ? 's f r x j' : '1 2 3 4 5'}</code><span>sắc · huyền · hỏi · ngã · nặng</span></p>
  </div><footer class="form-footer"><span class="muted">Nội dung không được lưu lại.</span><Action label="Đổi kiểu gõ" href={route(vm, 'settings')}/></footer></div>
}

const steps = {
  macos: [['Mở cài đặt Bàn phím', 'Cài đặt hệ thống → Bàn phím → Nhập văn bản → Sửa.'], ['Thêm nguồn nhập VNInput', 'Bấm dấu +, tìm Tiếng Việt và thêm VNInput.'], ['Chọn VNInput trên thanh menu', 'Mở menu nguồn nhập và chọn VNInput.']],
  windows: [['Mở cài đặt ngôn ngữ', 'Settings → Time & language → Language & region.'], ['Thêm bàn phím VNInput', 'Language options của Tiếng Việt → thêm VNInput.'], ['Chọn VNInput để bắt đầu', 'Nhấn Windows + Space và chọn VNInput.']],
  linux: [['Cài VNInput và Fcitx 5', 'Dùng gói VNInput dành cho bản phân phối của bạn.'], ['Thêm phương thức nhập', 'Mở Fcitx 5 Configuration và thêm VNInput.'], ['Chọn VNInput để bắt đầu', 'Chuyển sang VNInput trong menu Fcitx 5.']],
}
function Integration({ vm }: { vm: ViewModel }) {
  return <div class="page-form"><div class="form-body"><div class={'notice ' + (vm.ready ? 'success' : 'info')}><strong>{vm.ready ? 'VNInput đã sẵn sàng' : 'Thêm VNInput vào hệ thống'}</strong><span>{vm.ready ? 'Mở Gõ thử để kiểm tra kiểu gõ của bạn.' : 'Thực hiện một lần sau khi cài đặt.'}</span></div><ol class="setup-steps">{steps[vm.platform].map(([title,detail],i) => <li><span class={'step-number' + (vm.ready ? ' done' : '')}>{vm.ready ? '✓' : i+1}</span><div><strong>{title}</strong><p>{detail}</p></div></li>)}</ol></div><footer class="form-footer"><span class="muted">{platformNames[vm.platform]}</span><Action primary label={vm.ready ? 'Bắt đầu gõ thử' : 'Tôi đã chọn VNInput'} href={vm.ready ? route(vm, 'playground') : route(vm, 'integration', { state: 'ready' })}/></footer></div>
}
function About({ vm }: { vm: ViewModel }) {
  return <div class="page-form"><div class="form-body about-body"><div class="about-brand"><Brand/><span>0.1 · Bản xem trước</span></div><p class="tagline">Gõ tự nhiên.<br/>Viết tiếng mình.</p><dl class="about-facts"><div><dt>Gọn gàng</dt><dd>Chỉ mở thiết lập khi cần.</dd></div><div><dt>Riêng tư</dt><dd>Xử lý trên thiết bị, không lưu nội dung gõ.</dd></div><div><dt>Quen thuộc</dt><dd>Telex và VNI, trên cả ba hệ điều hành.</dd></div></dl></div><footer class="form-footer"><span class="muted">macOS · Windows · Linux</span><Action label="Hướng dẫn thiết lập" href={route(vm, 'integration')}/></footer></div>
}
function Restore({ vm }: { vm: ViewModel }) {
  return <div class="page-form"><div class="form-body restore-body"><span class="restore-symbol" aria-hidden="true">↺</span><h2>Khôi phục mặc định?</h2><p>Kiểu gõ trở về Telex.<br/>Phím chuyển trở về {formatShortcut('Alt + Z', vm.platform)}.</p></div><footer class="form-footer"><Action label="Giữ thiết lập hiện tại" quiet href={route(vm, vm.screen)}/><Action label="Khôi phục" primary href={route(vm, vm.screen, { state: 'restored', method: 'telex', shortcut: 'Alt + Z', tone: 'modern', 'free-tone':'1', 'restore-word':'1' })}/></footer></div>
}
function QuickMenu({ vm }: { vm: ViewModel }) {
  const row = (title: string, active: boolean, href: string, shortcut = '') => <a class="menu-item" href={href} role="menuitemradio" aria-checked={active}><span class="menu-check">{active ? '✓' : ''}</span><span>{title}</span>{shortcut && <kbd>{shortcut}</kbd>}</a>
  return <div class="quick-menu"><header><Brand/><span class="muted">{vm.method === 'telex' ? 'Telex' : 'VNI'}</span></header><div role="menu" aria-label="Chế độ gõ VNInput"><p class="menu-label">CHẾ ĐỘ GÕ</p>{row('Tiếng Việt',vm.enabled,route(vm,'quick-menu',{enabled:'1'}),formatShortcut(vm.shortcut, vm.platform))}{row('Tiếng Anh',!vm.enabled,route(vm,'quick-menu',{enabled:'0'}))}<div class="menu-divider"/>{row('Telex',vm.method==='telex',route(vm,'quick-menu',{method:'telex'}))}{row('VNI',vm.method==='vni',route(vm,'quick-menu',{method:'vni'}))}<div class="menu-divider"/><a class="menu-item" role="menuitem" href={route(vm,'settings')}><Icon name="settings"/><span>Thiết lập VNInput…</span><span class="menu-end">↗</span></a></div></div>
}
const labels: Record<ScreenId,[string,string]> = {
  settings:['Bộ gõ','Chọn cách gõ quen thuộc của bạn.'],
  shortcuts:['Phím tắt','Chuyển ngôn ngữ, giữ mạch suy nghĩ.'],
  playground:['Gõ thử','Thử dấu, sửa từ và làm quen kiểu gõ.'],
  integration:['Thiết lập hệ thống','Để VNInput sẵn sàng trong các ứng dụng.'],
  about:['Về VNInput','Một bộ gõ nhỏ, dành cho tiếng Việt.'],
  'quick-menu':['Menu nhanh',''],
}
export function VNInputPresenter({ vm }: { vm: ViewModel }) {
  if(vm.screen==='quick-menu') return <QuickMenu vm={vm}/>
  const content=vm.restoreConfirm?<Restore vm={vm}/>:vm.screen==='settings'?<Settings vm={vm}/>:vm.screen==='shortcuts'?<Shortcuts vm={vm}/>:vm.screen==='playground'?<Playground vm={vm}/>:vm.screen==='integration'?<Integration vm={vm}/>:<About vm={vm}/>
  return <div class="app"><header class="app-header"><Brand/><div class="header-status"><span class={'status-dot'+(!vm.enabled?' off':'')}/><span>{vm.enabled?'Tiếng Việt':'Tiếng Anh'}</span><a class="mode-button" title="Chuyển Việt / Anh" aria-label="Chuyển Việt / Anh" href={route(vm,vm.screen,{enabled:vm.enabled?'0':'1'})}>{vm.enabled?'V':'E'}</a></div></header><div class="app-layout"><aside class="sidebar"><nav aria-label="Thiết lập VNInput">{(['settings','shortcuts','playground','integration','about'] as ScreenId[]).map(id=><a class={'nav-item'+(id===vm.screen?' active':'')} href={route(vm,id)} aria-current={id===vm.screen?'page':undefined}><Icon name={id}/><span>{id==='integration'?'Hệ thống':labels[id][0]}</span></a>)}</nav><div class="sidebar-bottom"><a href={route(vm,'quick-menu')}>Menu V/E <span>↗</span></a><span>{platformNames[vm.platform]} · v0.1</span></div></aside><main class="main"><header class="page-heading"><h1>{labels[vm.screen][0]}</h1><p>{labels[vm.screen][1]}</p></header>{content}</main></div></div>
}
