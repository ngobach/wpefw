shortcut_map = {}           -- btn_id -> path
local path_to_btn = {}      -- path -> btn_id (reverse map)
local rendered_paths = {}   -- ordered list of all rendered paths
local last_snapshot = ""
local btn_counter = 0
local initialized = false

require 'winapi'
winapi.set_encoding(winapi.CP_UTF8)

local function get_shortcuts()
  local shortcuts = {}

  local user_programs = App.Call('startmenu::getprogramspath')
  if not user_programs or user_programs == "" then
    local appdata = App.Call('envstr', '%APPDATA%')
    user_programs = appdata .. [[\Microsoft\Windows\Start Menu\Programs]]
  end

  -- Use winapi.execute (hidden CreateProcess) instead of io.popen
  -- to avoid flashing CMD windows on screen
  local cmd = string.format('dir /s /b "%s\\*.lnk" 2>nul', user_programs)
  local ret, text = winapi.execute(cmd, 'ansi')
  if ret == 0 and text then
    for line in text:gmatch('[^\r\n]+') do
      if line ~= "" then
        table.insert(shortcuts, line)
      end
    end
  end

  -- Sort alphabetically by base name
  table.sort(shortcuts, function(a, b)
    local na = a:match("([^\\]+)$"):lower()
    local nb = b:match("([^\\]+)$"):lower()
    return na < nb
  end)

  return shortcuts
end

local function make_snapshot(shortcuts)
  return table.concat(shortcuts, "|")
end

local function get_base_name(path)
  -- Strip directory and .lnk extension (case-insensitive)
  return path:match("([^\\]+)%.lnk$") or path:match("([^\\]+)%.LNK$") or path:match("([^\\]+)$")
end

local function add_one_button(container, path, add_spacer)
  btn_counter = btn_counter + 1
  local btn_id = "dyn_btn_" .. btn_counter
  shortcut_map[btn_id] = path
  path_to_btn[path] = btn_id

  local name = get_base_name(path)
  local xml_path = path:gsub("\\", "/")
  local xml = string.format(
    [[<elem><Button name="%s" text="%s" style="menu_item" bkimage="file='%s,F' dest='12,5,44,37'" /></elem>]],
    btn_id, name, xml_path)
  container:add(xml)

  if add_spacer then
    container:add([[<elem><Control height="4" /></elem>]])
  end

  table.insert(rendered_paths, path)
end

local function populate_programs()
  local shortcuts = get_shortcuts()
  local container = sui:find('programs_list')
  if not container then return end

  for i, path in ipairs(shortcuts) do
    add_one_button(container, path, i < #shortcuts)
  end

  last_snapshot = make_snapshot(shortcuts)
end

local function refresh_programs()
  local shortcuts = get_shortcuts()
  local snapshot = make_snapshot(shortcuts)
  if snapshot == last_snapshot then return end  -- nothing changed

  local container = sui:find('programs_list')
  if not container then return end

  -- Build lookup sets
  local new_set = {}
  for _, p in ipairs(shortcuts) do new_set[p] = true end

  local rendered_set = {}
  for _, p in ipairs(rendered_paths) do rendered_set[p] = true end

  -- Hide buttons whose shortcut no longer exists
  for btn_id, path in pairs(shortcut_map) do
    if not new_set[path] then
      local btn = sui:find(btn_id)
      if btn then btn.visible = false end
    end
  end

  -- Append buttons for newly added shortcuts (sorted among themselves)
  for _, path in ipairs(shortcuts) do
    if not rendered_set[path] then
      add_one_button(container, path, true)
    end
  end

  last_snapshot = snapshot
end

function onload()
  populate_programs()
  initialized = true
end

function onshow()
  if not initialized then return end
  refresh_programs()
end

function onclick(ctrl)
  if shortcut_map[ctrl] then
    App:Run(shortcut_map[ctrl])
    sui:hide()
  elseif ctrl == "settings_btn" then
    wxsUI("UI_Settings", "main.jcfg", "-theme dark")
    sui:hide()
  elseif ctrl == "power_btn" then
    wxsUI("UI_Shutdown", "main.jcfg")
    sui:hide()
  end
end
