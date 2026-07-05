-- Load lib helper (contains base64 and JSON)
local lib = require("lib")

-- daemon.lua
-- Runs inside WinPE, connects to the host machine (10.0.2.2:8080)
-- Long-polls for JSON-RPC 2.0 commands, executes them, and returns stdout/stderr to the host

local HOST_IP = "10.0.2.2" -- QEMU Host Gateway
local PORT = 8080
local POLL_URL = string.format("http://%s:%d/poll", HOST_IP, PORT)
local RESPONSE_URL = string.format("http://%s:%d/response", HOST_IP, PORT)

-- Load Lua-cURL library
local has_curl, curl = pcall(require, "cURL")
if not has_curl then
  print("[Daemon] Error: Lua-cURL library ('cURL') could not be loaded!")
  os.exit(1)
end

-- Helper to perform HTTP GET request
local function http_get(url)
  local response_body = {}
  local c = curl.easy()
  c:setopt_url(url)
  c:setopt_timeout(35)
  c:setopt_writefunction(function(data)
    table.insert(response_body, data)
    return #data
  end)
  
  local ok, err = c:perform()
  c:close()
  
  if ok then
    return table.concat(response_body), nil
  else
    local err_code = type(err) == "table" and err.no and err:no()
    if err_code == 28 then
      return nil, "timeout"
    end
    return nil, "error"
  end
end

-- Helper to perform HTTP POST request
local function http_post(url, payload)
  local c = curl.easy()
  c:setopt_url(url)
  c:setopt_post(true)
  c:setopt_postfields(payload)
  c:setopt_httpheader({
    "Content-Type: application/json",
    "Content-Length: " .. tostring(#payload)
  })
  
  local ok, err = c:perform()
  c:close()
  return ok, err
end



local function execute_command(command)
  print("[Daemon] Running command: " .. command)
  local pipe = io.popen(command .. " 2>&1")
  if not pipe then
    return "Error: Failed to open pipe for execution", -1
  end
  
  local output = pipe:read("*all")
  local ok, exit_type, exit_code = pipe:close()
  
  local code = 0
  if not ok then
    code = tonumber(exit_code) or 1
  end
  
  return output, code
end

-- Specific Command Handlers
local function handle_import_reg(filepath)
  if not filepath then return nil, "Missing filepath" end
  local cmd = string.format('regedit.exe /s "%s"', filepath)
  local output, code = execute_command(cmd)
  return { success = (code == 0), exit_code = code, output = output }
end

local function handle_extract(filepath, dest_dir)
  if not filepath or not dest_dir then return nil, "Missing filepath or dest_dir" end
  
  -- Create destination directory
  os.execute(string.format('mkdir "%s" >nul 2>nul', dest_dir))
  
  local sys_drive = os.getenv("SystemDrive") or "X:"
  local seven_zip = sys_drive .. "\\Program Files\\easy7zip\\7z.exe"
  local cmd
  local f = io.open(seven_zip, "r")
  if f then
    f:close()
    cmd = string.format('"%s" x "%s" -o"%s" -y', seven_zip, filepath, dest_dir)
  else
    cmd = string.format('tar.exe -xf "%s" -C "%s"', filepath, dest_dir)
  end
  
  local output, code = execute_command(cmd)
  return { success = (code == 0), exit_code = code, output = output }
end

local function handle_remove_dir(dirpath)
  if not dirpath then return nil, "Missing dirpath" end
  local cmd = string.format('rmdir /s /q "%s"', dirpath)
  local output, code = execute_command(cmd)
  return { success = (code == 0), exit_code = code, output = output }
end

local function handle_read_file(filepath)
  if not filepath then return nil, "Missing filepath" end
  local f, err = io.open(filepath, "rb")
  if not f then
    return nil, "Failed to open file: " .. tostring(err)
  end
  local content = f:read("*all")
  f:close()
  return { success = true, content = lib.Base64.encode(content) }
end

local function handle_write_file(filepath, content_b64)
  if not filepath or not content_b64 then return nil, "Missing filepath or content" end
  local content = lib.Base64.decode(content_b64)
  local f, err = io.open(filepath, "wb")
  if not f then
    return nil, "Failed to open file for writing: " .. tostring(err)
  end
  f:write(content)
  f:close()
  return { success = true }
end

local function main()
  print("[Daemon] Windows PE Reverse JSON-RPC Daemon started.")
  print(string.format("[Daemon] Host Endpoint: http://%s:%d", HOST_IP, PORT))
  
  while true do
    local body, err = http_get(POLL_URL)
    
    if body then
      local cmd_id = lib.JSON:parseValue(body, "id")
      local method = lib.JSON:parseValue(body, "method")
      
      if cmd_id and method then
        local result, err_msg
        
        if method == "execute" then
          local command = lib.JSON:parseValue(body, "command")
          if command then
            local output, code = execute_command(command)
            result = { output = output, exit_code = code }
          else
            err_msg = "Missing command parameter"
          end
        elseif method == "import_reg" then
          local filepath = lib.JSON:parseValue(body, "filepath")
          result, err_msg = handle_import_reg(filepath)
        elseif method == "extract" then
          local filepath = lib.JSON:parseValue(body, "filepath")
          local dest_dir = lib.JSON:parseValue(body, "dest_dir")
          result, err_msg = handle_extract(filepath, dest_dir)
        elseif method == "remove_dir" then
          local dirpath = lib.JSON:parseValue(body, "dirpath")
          result, err_msg = handle_remove_dir(dirpath)
        elseif method == "read_file" then
          local filepath = lib.JSON:parseValue(body, "filepath")
          result, err_msg = handle_read_file(filepath)
        elseif method == "write_file" then
          local filepath = lib.JSON:parseValue(body, "filepath")
          local content = lib.JSON:parseValue(body, "content")
          result, err_msg = handle_write_file(filepath, content)
        else
          err_msg = "Method not found: " .. tostring(method)
        end
        
        local formatted_id = tonumber(cmd_id) and cmd_id or ('"' .. cmd_id .. '"')
        local response_payload
        
        if err_msg then
          response_payload = string.format(
            '{"jsonrpc":"2.0", "error":{"code":-32603, "message":"%s"}, "id":%s}',
            lib.JSON:escape(err_msg),
            formatted_id
          )
        else
          -- Serialize result table dynamically into JSON-RPC response
          local res_table = {}
          for k, v in pairs(result) do
            if type(v) == "boolean" then
              table.insert(res_table, string.format('"%s":%s', k, v and "true" or "false"))
            elseif type(v) == "number" then
              table.insert(res_table, string.format('"%s":%d', k, v))
            else
              table.insert(res_table, string.format('"%s":"%s"', k, lib.JSON:escape(tostring(v))))
            end
          end
          response_payload = string.format(
            '{"jsonrpc":"2.0", "result":{%s}, "id":%s}',
            table.concat(res_table, ", "),
            formatted_id
          )
        end
        
        -- Send response back to host
        local ok, post_err = http_post(RESPONSE_URL, response_payload)
        if not ok then
          print("[Daemon] Failed to send response: " .. tostring(post_err))
        end
      end
    elseif err == "timeout" then
      -- Long poll timed out; retry immediately
    else
      -- Network error. Wait and retry.
      lib.sleep(2)
    end
  end
end

-- Prevent GC of the script context
_G.remote_deploy_daemon_running = true

main()
