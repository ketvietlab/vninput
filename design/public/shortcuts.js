// Canonical values follow Qt PortableText; display follows the fixture's OS.
export function formatShortcut(portable, platform) {
  let key = portable.trim();
  const parts = [];
  let match;
  while ((match = /^(Meta|Alt|Shift|Ctrl)\s*\+\s*/.exec(key))) {
    parts.push(match[1]);
    key = key.slice(match[0].length);
  }
  if (platform !== 'macos') return [...parts, key].join(' + ');
  const modifiers = new Set(parts);
  return [
    modifiers.has('Meta') && '⌃', modifiers.has('Alt') && '⌥',
    modifiers.has('Shift') && '⇧', modifiers.has('Ctrl') && '⌘',
    key,
  ].filter(Boolean).join('');
}
