#!/usr/bin/env python3
import json
import time
import queue
import threading
from urllib.parse import urlparse
from socketserver import ThreadingMixIn
from http.server import HTTPServer, BaseHTTPRequestHandler

PORT = 8080

# JSON-RPC Broker State
rpc_queue = queue.Queue()
rpc_responses = {}
rpc_poll_event = threading.Event()
rpc_lock = threading.Lock()
active_clients = {}  # IP -> last_seen_timestamp

class ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

def add_log(msg):
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] {msg}")

class RPCBrokerRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        return

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        if path == "/poll":
            client_ip = self.client_address[0]
            with rpc_lock:
                active_clients[client_ip] = time.time()

            # Long poll: if queue is empty, wait up to 30 seconds
            if rpc_queue.empty():
                rpc_poll_event.wait(timeout=30.0)

            try:
                rpc_req = rpc_queue.get_nowait()
                if rpc_queue.empty():
                    rpc_poll_event.clear()
                self.send_json_response(rpc_req)
            except queue.Empty:
                self.send_json_response({"status": "timeout"})
            return

        if path == "/" or path == "/status":
            now = time.time()
            with rpc_lock:
                clients_list = [
                    f"{ip} ({'ONLINE' if now - last_seen < 45 else 'OFFLINE'})"
                    for ip, last_seen in active_clients.items()
                ]
            self.send_json_response({
                "status": "active",
                "role": "json-rpc-broker",
                "connected_clients": clients_list
            })
            return

        self.send_error(404)

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length) if content_length > 0 else b''

        payload = {}
        if post_data:
            try:
                payload = json.loads(post_data.decode('utf-8'))
            except Exception:
                pass

        # Route: Execute Command (from host)
        if self.path == "/execute":
            # Expects a standard JSON-RPC 2.0 Request
            req_id = payload.get("id")
            if req_id is None:
                self.send_json_response({
                    "jsonrpc": "2.0",
                    "error": {"code": -32600, "message": "Invalid Request: Missing 'id'"},
                    "id": None
                }, code=400)
                return

            event = threading.Event()
            with rpc_lock:
                rpc_responses[req_id] = {
                    "event": event,
                    "response": None
                }

            add_log(f"[Broker] Queueing JSON-RPC ID {req_id} (Method: {payload.get('method')})")
            rpc_queue.put(payload)
            rpc_poll_event.set()

            # Wait up to 60 seconds for the guest client response
            completed = event.wait(timeout=60.0)

            resp_data = None
            with rpc_lock:
                if req_id in rpc_responses:
                    resp_data = rpc_responses.pop(req_id)

            if completed and resp_data and resp_data["response"] is not None:
                self.send_json_response(resp_data["response"])
            else:
                self.send_json_response({
                    "jsonrpc": "2.0",
                    "error": {"code": -32603, "message": "Gateway Timeout: Guest execution timed out"},
                    "id": req_id
                }, code=504)
            return

        # Route: Response (from guest)
        if self.path == "/response":
            req_id = payload.get("id")
            if req_id is None:
                self.send_json_response({"status": "error", "message": "Missing 'id'"}, code=400)
                return

            with rpc_lock:
                if req_id in rpc_responses:
                    rpc_responses[req_id]["response"] = payload
                    rpc_responses[req_id]["event"].set()
                    add_log(f"[Broker] Forwarded response for JSON-RPC ID {req_id}")

            self.send_json_response({"status": "ok"})
            return

        self.send_error(404)

    def send_json_response(self, obj, code=200):
        data = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

def run():
    add_log("Starting Repurposed RPC Broker Server...")
    server = ThreadingHTTPServer(('0.0.0.0', PORT), RPCBrokerRequestHandler)
    add_log(f"Server is listening at http://localhost:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    add_log("Server stopped.")

if __name__ == "__main__":
    run()
