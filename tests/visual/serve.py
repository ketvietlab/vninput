"""Serve unmodified Qt/browser captures for KetPlus raster comparison only.

The identical HTML shells expose images, not native widget semantics.
Native behavior and accessibility need separate Qt tests.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=61742)
parser.add_argument('--root', type=Path, default=Path('design/evidence/qt'))
args = parser.parse_args()
root = args.root.resolve()
pages = {'settings', 'shortcuts', 'playground', 'integration', 'about', 'quick-menu', 'restore'}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        url = urlparse(self.path)
        query = parse_qs(url.query)
        side = query.get('side', ['reference'])[0]
        page = query.get('page', ['settings'])[0]
        if side not in {'reference', 'candidate'} or page not in pages:
            self.send_error(400)
            return
        if url.path == '/capture.png':
            path = root / side / (page + '.png')
            if not path.is_file():
                self.send_error(404)
                return
            data = path.read_bytes()
            mime = 'image/png'
        elif url.path == '/':
            data = (f'<!doctype html><html lang="vi"><head><meta charset="utf-8">'
                    f'<title>VNInput visual evidence</title><style>html,body{{margin:0;background:white}}'
                    f'img{{display:block}}</style></head><body>'
                    f'<img id="native-capture" alt="VNInput interface capture" '
                    f'src="/capture.png?side={side}&amp;page={page}"></body></html>').encode()
            mime = 'text/html; charset=utf-8'
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header('Content-Type', mime)
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(data)))
        self.end_headers()
        self.wfile.write(data)
    def log_message(self, *args):
        pass

HTTPServer(('127.0.0.1', args.port), Handler).serve_forever()
