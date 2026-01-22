#!/usr/bin/env python3
"""
Simple HTTP server for testing WASM module
Serves files with proper MIME types for WebAssembly
"""

import http.server
import socketserver
import os

PORT = 8080

class WasmHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for local testing
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', '*')

        # Enable cross-origin isolation for SharedArrayBuffer support
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')

        # Set proper MIME type for WASM files
        if self.path.endswith('.wasm'):
            self.send_header('Content-Type', 'application/wasm')

        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

if __name__ == '__main__':
    # Change to script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    print(f"========================================")
    print(f"  WASM Test Server")
    print(f"========================================")
    print(f"Server running at: http://localhost:{PORT}")
    print(f"Test page: http://localhost:{PORT}/test.html")
    print(f"")
    print(f"Press Ctrl+C to stop the server")
    print(f"========================================\n")

    with socketserver.TCPServer(("", PORT), WasmHTTPRequestHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n\nServer stopped.")
