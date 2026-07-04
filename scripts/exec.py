#!/usr/bin/env python3
import sys
import json
import time
import urllib.request

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 exec.py \"<command>\"")
        sys.exit(1)
        
    command = sys.argv[1]
    url = "http://localhost:8080/execute"
    
    # Formulate JSON-RPC 2.0 request
    req_id = int(time.time() * 1000)
    rpc_request = {
        "jsonrpc": "2.0",
        "method": "execute",
        "params": {
            "command": command
        },
        "id": req_id
    }
    
    payload = json.dumps(rpc_request).encode("utf-8")
    
    req = urllib.request.Request(
        url, 
        data=payload, 
        headers={"Content-Type": "application/json"}
    )
    
    try:
        with urllib.request.urlopen(req, timeout=65) as response:
            res_data = json.loads(response.read().decode("utf-8"))
            
            # Check for JSON-RPC error
            if "error" in res_data:
                err = res_data["error"]
                print(f"RPC Error ({err.get('code')}): {err.get('message')}", file=sys.stderr)
                sys.exit(1)
                
            # Parse result
            result = res_data.get("result", {})
            output = result.get("output", "")
            exit_code = result.get("exit_code", 0)
            
            print(output, end="")
            sys.exit(exit_code)
            
    except Exception as e:
        print(f"HTTP Request failed: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
