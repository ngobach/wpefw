local ffi = require("ffi")
ffi.cdef[[
  int MessageBoxA(void *hWnd, const char *lpText, const char *lpCaption, unsigned int uType);
  void Sleep(unsigned int dwMilliseconds);
]]

local M = {}

function M.MessageBox(text, caption, uType)
  ffi.C.MessageBoxA(nil, text, caption or "Message", uType or 0)
end

-- Base64 character table
local b = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

M.Base64 = {}

-- Base64 encoder
function M.Base64.encode(data)
  local padding = 0
  local to_encode = data
  while #to_encode % 3 ~= 0 do
    to_encode = to_encode .. '\0'
    padding = padding + 1
  end
  local result = {}
  for i = 1, #to_encode, 3 do
    local b1, b2, b3 = to_encode:byte(i, i + 2)
    local n = b1 * 65536 + b2 * 256 + b3
    local c1 = math.floor(n / 262144) % 64
    local c2 = math.floor(n / 4096) % 64
    local c3 = math.floor(n / 64) % 64
    local c4 = n % 64
    table.insert(result, b:sub(c1+1, c1+1) .. b:sub(c2+1, c2+1) .. b:sub(c3+1, c3+1) .. b:sub(c4+1, c4+1))
  end
  local s = table.concat(result)
  if padding == 1 then
    s = s:sub(1, -2) .. '='
  elseif padding == 2 then
    s = s:sub(1, -3) .. '=='
  end
  return s
end

-- Base64 decoder
function M.Base64.decode(data)
  data = string.gsub(data, '[^'..b..'=]', '')
  local result = {}
  local padding = 0
  if data:sub(-2) == '==' then
    padding = 2
  elseif data:sub(-1) == '=' then
    padding = 1
  end
  local clean_data = data:gsub('=', 'A')
  for i = 1, #clean_data, 4 do
    local c1 = b:find(clean_data:sub(i, i)) - 1
    local c2 = b:find(clean_data:sub(i+1, i+1)) - 1
    local c3 = b:find(clean_data:sub(i+2, i+2)) - 1
    local c4 = b:find(clean_data:sub(i+3, i+3)) - 1
    local n = c1 * 262144 + c2 * 4096 + c3 * 64 + c4
    local b1 = math.floor(n / 65536) % 256
    local b2 = math.floor(n / 256) % 256
    local b3 = n % 256
    table.insert(result, string.char(b1, b2, b3))
  end
    local s = table.concat(result)
  if padding > 0 then
    s = s:sub(1, -1 - padding)
  end
  return s
end

-- Helper to sleep using Win32 API Sleep
function M.sleep(sec)
  ffi.C.Sleep(sec * 1000)
end

-- JSON Helpers namespace
M.JSON = {}

function M.JSON:escape(s)
  if not s then return "" end
  return s:gsub("\\", "\\\\"):gsub("\"", "\\\""):gsub("\n", "\\n"):gsub("\r", "\\r"):gsub("\t", "\\t")
end

function M.JSON:parseValue(json, key)
  local num = json:match('"' .. key .. '"%s*:%s*(%d+)')
  if num then return num end
  
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

return M
