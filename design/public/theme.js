(() => {
  let requested;
  let original;
  const apply = () => {
    const root = document.getElementById('vninput-root');
    if (!root) return;
    original ??= { theme: root.getAttribute('data-theme'), scheme: document.documentElement.style.colorScheme };
    if (requested === undefined) {
      document.documentElement.style.colorScheme = original.theme || 'light';
      return;
    }
    const theme = requested ?? original.theme;
    if (theme === null) root.removeAttribute('data-theme');
    else root.setAttribute('data-theme', theme);
    document.documentElement.style.colorScheme = requested ?? (original.scheme || original.theme || '');
  };
  window.addEventListener('message', (event) => {
    if (event.source !== window.parent || event.data?.type !== 'ketatlas:theme') return;
    if (!['light', 'dark', null].includes(event.data.theme)) return;
    requested = event.data.theme;
    apply();
  });
  document.addEventListener('DOMContentLoaded', () => {
    apply();
    if (window.parent !== window) window.parent.postMessage({ type: 'ketatlas:theme-ready' }, '*');
  }, { once: true });
})();
