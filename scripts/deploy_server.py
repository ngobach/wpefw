#!/usr/bin/env python3
import os
import zipfile
import json
import time
import threading
from urllib.parse import urlparse, parse_qs
from socketserver import ThreadingMixIn
from http.server import HTTPServer, BaseHTTPRequestHandler
from io import BytesIO

PORT = 8080
latest_version = 0
deploy_event = threading.Event()
payload_zip_data = None
SANDBOX_PATH = "/Users/bachnx/Projects/wpefw/sandbox"

lock = threading.Lock()
active_clients = {}  # IP -> last_seen_timestamp

class ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
    daemon_threads = True

def add_log(msg):
    timestamp = time.strftime("%H:%M:%S")
    print(f"[{timestamp}] {msg}")

def trigger_deploy():
    global latest_version, payload_zip_data
    if not os.path.exists(SANDBOX_PATH):
        add_log(f"Error: Sandbox path does not exist: {SANDBOX_PATH}")
        return False
    
    zip_buffer = BytesIO()
    file_count = 0
    with zipfile.ZipFile(zip_buffer, 'w', zipfile.ZIP_DEFLATED) as zip_file:
        for root, dirs, files in os.walk(SANDBOX_PATH):
            if '.git' in root or '__pycache__' in root:
                continue
            for file in files:
                if file.startswith('.'):
                    continue
                file_path = os.path.join(root, file)
                rel_path = os.path.relpath(file_path, SANDBOX_PATH)
                zip_file.write(file_path, rel_path)
                file_count += 1
                
    with lock:
        payload_zip_data = zip_buffer.getvalue()
        latest_version += 1
    
    add_log(f"Packed version {latest_version} from '{SANDBOX_PATH}' ({file_count} files, {len(payload_zip_data)} bytes)")
    
    # Notify all blocking long-poll requests
    deploy_event.set()
    deploy_event.clear()
    return True

# Background file watcher to deploy on change
def file_watcher_thread():
    global auto_deploy_enabled, current_watch_path
    last_mtime = 0
    stable_mtime = 0
    stable_checks = 0
    
    while True:
        if os.path.exists(SANDBOX_PATH):
            max_mtime = 0
            for root, dirs, files in os.walk(SANDBOX_PATH):
                if '.git' in root or '__pycache__' in root:
                    continue
                for f in files:
                    if f.startswith('.'):
                        continue
                    try:
                        mtime = os.path.getmtime(os.path.join(root, f))
                        if mtime > max_mtime:
                            max_mtime = mtime
                    except OSError:
                        pass
            
            if last_mtime == 0:
                last_mtime = max_mtime
            elif max_mtime > last_mtime:
                if max_mtime == stable_mtime:
                    stable_checks += 1
                else:
                    stable_mtime = max_mtime
                    stable_checks = 0
                
                if stable_checks >= 2:  # Stable for ~1 second
                    add_log("[Watcher] Sandbox changes detected. Auto-deploying...")
                    last_mtime = max_mtime
                    stable_mtime = 0
                    stable_checks = 0
                    trigger_deploy()
        else:
            stable_checks = 0
            stable_mtime = 0
            
        time.sleep(0.5)

class DeployRequestHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        return

    def do_GET(self):
        global latest_version, payload_zip_data, active_clients
        
        parsed_url = urlparse(self.path)
        path = parsed_url.path
        
        # Route: API Poll
        if path == "/poll":
            client_ip = self.client_address[0]
            with lock:
                active_clients[client_ip] = time.time()
            
            query = parse_qs(parsed_url.query)
            client_version = 0
            if "version" in query:
                try:
                    client_version = int(query["version"][0])
                except ValueError:
                    pass
            
            with lock:
                current_latest = latest_version
                
            if current_latest > client_version:
                self.send_json_response({"version": current_latest})
                return
            
            got_signal = deploy_event.wait(timeout=30.0)
            with lock:
                current_latest = latest_version
                
            if got_signal and current_latest > client_version:
                self.send_json_response({"version": current_latest})
            else:
                self.send_json_response({"version": client_version, "status": "timeout"})
            return

        # Route: Download payload
        if path == "/download":
            with lock:
                data = payload_zip_data
            if data is None:
                self.send_error(404, "No payload available")
                return
            self.send_response(200)
            self.send_header("Content-Type", "application/zip")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return

        # Simple status on root
        if path == "/" or path == "/status":
            now = time.time()
            with lock:
                clients_list = [
                    f"{ip} ({'ONLINE' if now - last_seen < 45 else 'OFFLINE'})"
                    for ip, last_seen in active_clients.items()
                ]
                ver = latest_version
            
            self.send_json_response({
                "status": "active",
                "version": ver,
                "sandbox": SANDBOX_PATH,
                "connected_clients": clients_list
            })
            return

        self.send_error(404)

    def do_POST(self):
        if self.path == "/deploy":
            add_log("[API] Manual deployment triggered via /deploy POST request.")
            success = trigger_deploy()
            if success:
                self.send_json_response({"status": "ok", "version": latest_version})
            else:
                self.send_json_response({"status": "error", "message": "Failed to pack sandbox"}, code=500)
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
    add_log("Starting Simplified Remote Deploy Server...")
    add_log(f"Target Sandbox Path: {SANDBOX_PATH}")
    
    # Trigger initial deployment of current sandbox content on startup
    trigger_deploy()
    
    # Start file watcher
    watcher_thread = threading.Thread(target=file_watcher_thread, daemon=True)
    watcher_thread.start()
    
    server = ThreadingHTTPServer(('0.0.0.0', PORT), DeployRequestHandler)
    add_log(f"Server is listening at http://localhost:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    add_log("Server stopped.")

if __name__ == "__main__":
    run()
