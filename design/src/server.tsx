import { compose, createKetServer, document, json, page, sqliteAdapter } from '@ketvietlab/ketjs'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import { fixture, screenIds, type ScreenId } from './fixtures.ts'
import { VNInputPresenter } from './presenters.tsx'

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..')
const ds = dirname(fileURLToPath(import.meta.resolve('@ketvietlab/design-system')))
const args = process.argv.slice(2)
const argument = (key: string, fallback: string) => args.includes(key) ? args[args.indexOf(key) + 1] || fallback : fallback
const host = argument('--host', process.env.KETATLAS_HOST || '127.0.0.1')
const port = Number(argument('--port', process.env.KETATLAS_HTML_PORT || '61741'))
if (host !== '127.0.0.1' && host !== 'localhost') throw new Error('VNInput design renderer is loopback-only')

const render = (url: URL, screen: ScreenId) => {
  const vm = fixture(url, screen)
  return page({ body: document({
    lang: 'vi',
    title: 'VNInput · ' + screen,
    head: <><meta name="viewport" content="width=device-width, initial-scale=1"/><link rel="stylesheet" href="/design-system/foundations/tokens.css"/><link rel="stylesheet" href="/assets/vninput.css"/><script src="/assets/theme.js"/></>,
    body: <><div id="vninput-root" data-theme={vm.theme} data-platform={vm.platform} data-screen={screen} data-method={vm.method}><VNInputPresenter vm={vm}/></div><script type="module" src="/assets/interactions.js"/></>,
  }) })
}
const adapter = sqliteAdapter()
await adapter.open()
const app = await createKetServer({
  adapter,
  manifest: compose([], { headless: true }),
  assets: [{ prefix: '/design-system/', dir: ds }, { prefix: '/assets/', dir: resolve(root, 'public') }],
  routes: {
    '/__atlas/ready': () => json({ ready: true, framework: 'ketjs', designSystem: '0.1.29' }),
    '/': url => render(url, 'settings'),
    ...Object.fromEntries(screenIds.map(id => ['/__atlas/vninput/' + id, (url: URL) => render(url, id)])),
  },
})
await new Promise<void>(done => app.server.listen(port, host, done))
console.log('VNInput KetJS renderer: http://' + host + ':' + port)
const close = async () => { await app.close(); process.exit(0) }
process.on('SIGTERM', close)
process.on('SIGINT', close)
