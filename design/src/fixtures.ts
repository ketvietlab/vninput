export const screenIds = ['settings', 'shortcuts', 'playground', 'integration', 'about', 'quick-menu'] as const
export type ScreenId = typeof screenIds[number]
export type Platform = 'macos' | 'windows' | 'linux'
export type ViewModel = {
  screen: ScreenId
  platform: Platform
  method: 'telex' | 'vni'
  enabled: boolean
  saved: boolean
  restored: boolean
  conflict: boolean
  shortcut: string
  tone: string
  modern: boolean
  restoreWord: boolean
  restoreConfirm: boolean
  ready: boolean
  sample: boolean
  menuOpen: boolean
  theme: 'light' | 'dark'
}
export const platformNames: Record<Platform, string> = { macos: 'macOS', windows: 'Windows', linux: 'Linux' }
export function fixture(url: URL, screen: ScreenId): ViewModel {
  const p = url.searchParams
  const platform = p.get('platform')
  const saved = p.get('saved') === '1'
  const shortcut = p.get('shortcut') || 'Alt + Z'
  return {
    screen,
    platform: platform === 'windows' || platform === 'linux' ? platform : 'macos',
    method: p.get('method') === 'vni' ? 'vni' : 'telex',
    enabled: p.get('enabled') !== '0',
    saved,
    restored: p.get('state') === 'restored',
    conflict: p.get('state') === 'conflict' || (saved && shortcut === 'Ctrl + Space'),
    shortcut,
    tone: p.get('tone') || 'modern',
    modern: p.has('free-tone') ? p.get('free-tone') === '1' : !saved,
    restoreWord: p.has('restore-word') ? p.get('restore-word') === '1' : !saved,
    restoreConfirm: p.get('state') === 'restore',
    ready: p.get('state') === 'ready',
    sample: p.get('state') === 'sample',
    menuOpen: p.get('open') !== '0',
    theme: p.get('theme') === 'dark' ? 'dark' : 'light',
  }
}
export function route(vm: ViewModel, screen: ScreenId, extra: Record<string, string> = {}) {
  const p = new URLSearchParams({
    platform: vm.platform, method: vm.method, enabled: vm.enabled ? '1' : '0',
    tone: vm.tone, shortcut: vm.shortcut,
    'free-tone': vm.modern ? '1' : '0', 'restore-word': vm.restoreWord ? '1' : '0', ...extra,
  })
  return '/__atlas/vninput/' + screen + '?' + p.toString()
}
