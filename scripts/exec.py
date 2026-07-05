#!/usr/bin/env python3
import sys
import json
import time
import base64
import urllib.request

def usage():
    print("Usage:")
    print("  python3 exec.py execute \"<command>\"")
    print("  python3 exec.py import_reg \"<remote_reg_path>\"")
    print("  python3 exec.py extract \"<remote_zip_path>\" \"<remote_dest_dir>\"")
    print("  python3 exec.py remove_dir \"<remote_dir_path>\"")
    print("  python3 exec.py read_file \"<remote_file_path>\" [local_output_path]")
    print("  python3 exec.py write_file \"<remote_file_path>\" \"<local_source_path>\"")
    sys.exit(1)

def main():
    if len(sys.argv) < 3:
        usage()
        
    method = sys.argv[1]
    url = "http://localhost:8080/execute"
    
    # Map command line to JSON-RPC 2.0 params
    params = {}
    
    if method == "execute":
        params["command"] = sys.argv[2]
    elif method == "import_reg":
        params["filepath"] = sys.argv[2]
    elif method == "extract":
        if len(sys.argv) < 4:
            print("Error: Missing <remote_dest_dir>", file=sys.stderr)
            usage()
        params["filepath"] = sys.argv[2]
        params["dest_dir"] = sys.argv[3]
    elif method == "remove_dir":
        params["dirpath"] = sys.argv[2]
    elif method == "read_file":
        params["filepath"] = sys.argv[2]
    elif method == "write_file":
        if len(sys.argv) < 4:
            print("Error: Missing <local_source_path>", file=sys.stderr)
            usage()
        local_path = sys.argv[3]
        try:
            with open(local_path, "rb") as f:
                file_bytes = f.read()
                params["content"] = base64.b64encode(file_bytes).decode("utf-8")
        except Exception as e:
            print(f"Error reading local file '{local_path}': {e}", file=sys.stderr)
            sys.exit(1)
        params["filepath"] = sys.argv[2]
    else:
        # Fallback: if user specified only one argument for legacy command format, treat it as execute
        if len(sys.argv) == 2:
            method = "execute"
            params["command"] = sys.argv[1]
        else:
            usage()

    req_id = int(time.time() * 1000)
    rpc_request = {
        "jsonrpc": "2.0",
        "method": method,
        "params": params,
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
            
            if "error" in res_data:
                err = res_data["error"]
                print(f"RPC Error ({err.get('code')}): {err.get('message')}", file=sys.stderr)
                sys.exit(1)
                
            result = res_data.get("result", {})
            
            # Post-processing for special methods
            if method == "read_file":
                content_b64 = result.get("content", "")
                try:
                    file_bytes = base64.b64decode(content_b64)
                except Exception as e:
                    print(f"Error decoding base64 response: {e}", file=sys.stderr)
                    sys.exit(1)
                    
                if len(sys.argv) >= 4:
                    out_path = sys.argv[3]
                    try:
                        with open(out_path, "wb") as f:
                            f.write(file_bytes)
                        print(f"File successfully saved to '{out_path}'")
                    except Exception as e:
                        print(f"Error saving file to '{out_path}': {e}", file=sys.stderr)
                        sys.exit(1)
                else:
                    # Print text to stdout, fallback to printing repr for binary
                    try:
                        print(file_bytes.decode("utf-8"), end="")
                    except UnicodeDecodeError:
                        sys.stdout.buffer.write(file_bytes)
                sys.exit(0)
                
            elif method == "execute":
                print(result.get("output", ""), end="")
                sys.exit(result.get("exit_code", 0))
                
            else:
                success = result.get("success")
                if success:
                    print(f"RPC Method '{method}' completed successfully.")
                    if "output" in result and result["output"]:
                        print(result["output"])
                    sys.exit(0)
                else:
                    print(f"RPC Method '{method}' failed: {result.get('output', 'Unknown error')}", file=sys.stderr)
                    sys.exit(result.get("exit_code", 1))
                    
    except Exception as e:
        print(f"HTTP Request failed: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
