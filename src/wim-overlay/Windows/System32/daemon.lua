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

-- Helper to sleep using ping
local function sleep(sec)
  os.execute(string.format("ping.exe -n %d 127.0.0.1 >nul", sec + 1))
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

-- Custom json escape function for strings
local function json_escape(s)
  if not s then return "" end
  return s:gsub("\\", "\\\\"):gsub("\"", "\\\""):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
end

-- Robust JSON value parser
local function parse_json_value(json, key)
  -- Try to match a numeric value (e.g. "id": 12345)
  local num = json:match('"' .. key .. '"%s*:%s*(%d+)')
  if num then return num end
  
  -- Try to match a string value (e.g. "command": "value")
  local pattern = '"' .. key .. '"%s*:%s*"'
  local start_idx = json:find(pattern)
  if not start_idx then return nil end
  local val_start = start_idx + json:match(pattern):len()
  local chars = {}
  local i = val_start
  while i <= #json do
    local c = json:sub(i, i)
    if c == '"' then
      break
    elseif c == '\\' then
      local next_c = json:sub(i+1, i+1)
      if next_c == '"' then
        table.insert(chars, '"')
        i = i + 2
      elseif next_c == '\\' then
        table.insert(chars, '\\')
        i = i + 2
      else
        table.insert(chars, c)
        i = i + 1
      end
    else
      table.insert(chars, c)
      i = i + 1
    end
  end
  return table.concat(chars)
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

local function main()
  print("[Daemon] Windows PE Reverse JSON-RPC Daemon started.")
  print(string.format("[Daemon] Host Endpoint: http://%s:%d", HOST_IP, PORT))
  
  while true do
    local body, err = http_get(POLL_URL)
    
    if body then
      local cmd_id = parse_json_value(body, "id")
      local method = parse_json_value(body, "method")
      local command = parse_json_value(body, "command")
      
      if cmd_id and method == "execute" and command then
        local output, exit_code = execute_command(command)
        
        -- Determine if ID needs quoting (standard numeric IDs do not)
        local formatted_id = tonumber(cmd_id) and cmd_id or ('"' .. cmd_id .. '"')
        
        -- Build JSON-RPC 2.0 Response
        local response_payload = string.format(
          '{"jsonrpc":"2.0", "result":{"output":"%s", "exit_code":%d}, "id":%s}',
          json_escape(output),
          exit_code,
          formatted_id
        )
        
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
      sleep(2)
    end
  end
end

-- Prevent GC of the script context
_G.remote_deploy_daemon_running = true

main()
