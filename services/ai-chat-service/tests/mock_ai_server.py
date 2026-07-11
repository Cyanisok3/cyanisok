from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import time


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("content-length", "0"))
        payload = json.loads(self.rfile.read(length) or b"{}")
        if payload.get("max_tokens") != 4096:
            self.send_response(400)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error":{"message":"unexpected max_tokens"}}')
            return

        messages = payload.get("messages", [])
        question = messages[-1].get("content", "") if messages else ""
        if question == "fail":
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"error":{"message":"mock failure"}}')
            return

        if question.startswith("hold"):
            time.sleep(1.5)

        answer = f"mock response: {question} (context={len(messages)})"

        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()

        midpoint = max(1, len(answer) // 2)
        for index, chunk in enumerate((answer[:midpoint], answer[midpoint:])):
            if question == "disconnect" and index == 1:
                time.sleep(1.5)
            event = {"choices": [{"delta": {"content": chunk}}]}
            try:
                self.wfile.write(f"data: {json.dumps(event)}\n\n".encode())
                self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                return
        if question == "truncated":
            return
        try:
            self.wfile.write(b"data: [DONE]\n\n")
            self.wfile.flush()
        except (BrokenPipeError, ConnectionResetError):
            return

    def log_message(self, format, *args):
        return


ThreadingHTTPServer(("0.0.0.0", 8080), Handler).serve_forever()
