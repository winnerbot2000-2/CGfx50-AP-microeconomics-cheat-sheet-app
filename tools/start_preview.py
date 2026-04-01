from __future__ import annotations

import http.server
import socketserver
import threading
import webbrowser
from pathlib import Path


CALC_ROOT = Path(__file__).resolve().parent.parent
HOST = "localhost"
PORT = 8123


def main() -> None:
    handler = http.server.SimpleHTTPRequestHandler
    with socketserver.TCPServer((HOST, PORT), handler) as httpd:
        thread = threading.Thread(target=httpd.serve_forever, daemon=True)
        thread.start()
        url = f"http://{HOST}:{PORT}/preview/index.html"
        print(f"Serving preview from {CALC_ROOT}")
        print(f"Open {url}")
        webbrowser.open(url)
        try:
            thread.join()
        except KeyboardInterrupt:
            print("\nStopping preview server...")
            httpd.shutdown()


if __name__ == "__main__":
    import os

    os.chdir(CALC_ROOT)
    main()
