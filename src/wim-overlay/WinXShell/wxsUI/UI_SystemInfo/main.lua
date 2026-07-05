local fallback_strings = {
  ["systemcpl.dll"] = {
    [1] = "System",
    [100] = "Device Manager",
    [101] = "Remote settings",
    [102] = "System protection",
    [103] = "Advanced system settings",
    [104] = "Windows Update",
    [105] = "Activation",
    [106] = "Performance Information and Tools",
    [45] = "%s (%s usable)",
    [1537] = "View basic information about your computer",
    [1538] = "Windows edition",
    [1542] = "System",
    [1562] = "Processor:",
    [1564] = "Installed memory (RAM):",
    [1571] = "System type:",
    [1572] = "Manufacturer:",
    [1574] = "Model:",
    [23] = "32-bit Operating System",
    [24] = "64-bit Operating System",
    [56] = "32-bit Operating System, x86-based processor",
    [60] = "64-bit Operating System, x64-based processor",
    [1567] = "64-bit Operating System, x64-based processor",
    [1568] = "32-bit Operating System, x86-based processor",
    [1579] = "64-bit Operating System, ARM-based processor",
    [1569] = "No Pen or Touch Input is available for this Display",
    [1545] = "Computer name, domain, and workgroup settings",
    [1546] = "Change settings",
    [1547] = "Computer name:",
    [1549] = "Full computer name:",
    [1551] = "Computer description:",
    [1553] = "Workgroup:"
  },
  ["shell32.dll"] = {
    [31057] = "Control Panel Home",
    [31091] = "See also",
    [8981] = "activation",
    [9307] = "Available:",
    [25379] = "System uptime: %1!d! days, %2!d! hours, %3!d! minutes, %4!d! seconds",
    [28876] = "%1!d! hours",
    [28879] = "%1!d! minutes",
    [28882] = "%1!d! seconds"
  }
}

local function get_res_string(id, file)
  file = file or 'systemcpl.dll'
  local res_path = str.format('#{@%s,%s}', file, id)
  local val = string.resstr(res_path)
  if not val or val == "" or val == res_path then
    local fallback_file = fallback_strings[file:lower()]
    if fallback_file and fallback_file[id] then
      val = fallback_file[id]
    else
      val = ""
    end
  end
  return val
end

local function set_res(name, id, file)
  sui:find(name).text = get_res_string(id, file)
end

local function set_text(name, text)
  sui:find(name).text = text
end

timetick_format = '%02d:%02d:%02d'
runtime_text = nil

local function update_timetick()
  -- body
  local tick = suilib.call('GetTickCount')
  tick = math.floor(tick / 1000) -- sec
  local h = math.floor(tick / 3600)
  local m = math.floor((tick - h * 3600) / 60)
  local s = tick % 60
  local t = string.format(timetick_format, h, m, s)
  runtime_text.text = t
end

function UIWindow.OnLoad()
  --p(os_ver_info()['ProductName'])
  --p(win_copyright())

  local vernum = os.info('winver')['1.2']

  if sui:jcfg('usesystemcaption') then
    sui:find('::caption').visible = 0
  end

  sui:title(get_res_string(1))
  sui:find('::title').text = get_res_string(1)
  sui:find('cphome_btn').text = get_res_string(31057, 'shell32.dll')
  sui:find('devmgmt_btn').text = get_res_string(100)
  sui:find('rmt_btn').text = get_res_string(101)
  sui:find('protect_btn').text = get_res_string(102)
  sui:find('adv_btn').text = get_res_string(103)
  sui:find('act_btn').text = get_res_string(105)
  sui:find('more_label').text = get_res_string(31091, 'shell32.dll')
  sui:find('update_btn').text = get_res_string(104)

  if vernum == '6.1' then  -- Windows 7
    local perf_btn = sui:find('perf_btn')
    perf_btn.text = get_res_string(106)
    perf_btn.visible = 1
  end

  set_res('basic_label', 1537)
  set_res('edition_group',1538)

  local ver_info = os_ver_info()

  if ver_info then -- need Administrator's right privilege to read the HKLM items
    local ver_res = '#{@Branding\\Basebrd\\basebrd.dll,14}'
    local ver_text = string.resstr(ver_res)
    if not ver_text or ver_text == "" or ver_text == ver_res then
      ver_text = "Windows 11"
    end
    set_text('edition_edit1', ver_info['ProductName'] or "Windows 11 Preinstallation Environment")
    set_text('edition_edit2', ver_text)
    if (ver_info['CSDVersion']) then -- Windows 10 hasn't 'Service Pack x'
      set_text('edition_edit3', ver_info['CSDVersion'])
    else
      local rsN = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
        , 'BuildBranch')
      if rsN then set_text('edition_edit3', rsN) end
    end
  end

  local win_logo = 'windows'
  if vernum == '6.1' or vernum == '10.0' or vernum == '11.0' then
    win_logo = 'win_' .. vernum
  end

  if vernum == '10.0' then
    local current_build = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
        , 'CurrentBuild')
    if tonumber(current_build) >= 22000 then win_logo = 'win_11.0' end
  end

  local installation_type = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion]]
    , 'InstallationType')
  if installation_type and installation_type == 'Server' then
    win_logo = 'win_server'
  end
  sui:find('brand_img').bkimage = "file='" .. win_logo .. ".png'"

  set_res('system_group', 1542)
  set_res('mfgr_label', 1572)
  set_res('model_label', 1574)
  set_res('cpu_label', 1562)
  set_res('mem_label', 1564)
  set_res('arch_label',1571)

  local need_si_height = 0
  local mfgr_info = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation]]
        , 'Manufacturer')

  if mfgr_info then
    need_si_height = 20
    set_text('mfgr_text', mfgr_info)
  else
     sui:find('mfgr_label').visible = 0
     sui:find('mfgr_text').visible = 0
  end

  local model_info = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation]]
        , 'Model')

  if model_info then
    need_si_height = need_si_height + 20
    set_text('model_text', model_info)
  else
     sui:find('model_label').visible = 0
     sui:find('model_text').visible = 0
  end

  local oem_logo = Reg:Read([[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\OEMInformation]]
        , 'Logo')
  if oem_logo then
    sui:find('oem_logo').bkimage = "file='" .. oem_logo .. "'"
  end

  if need_si_height > 0 then
    sui:find('system_info').height = 120 + need_si_height
  end

  local cpu_t = os.info('CPU')

  if cpu_t then
    local cpu_f = cpu_t['~MHz']
    cpu_f = cpu_f / 1000
    cpu_str = str.format("%s %.2fGHz", cpu_t['name'], cpu_f)
    set_text('cpu_text', cpu_str)
  end

  local mem_format = get_res_string(45)

  local mem_t = os.info('MEM')
  local total_phys = str.format("%.2f GB", mem_t['total_gb'])

  local avail_phys = mem_t['total'] / 0x40000000 -- 1024*1024*1024
  avail_phys = str.format("%.2f GB", avail_phys)

  local now_avail = str.format("%s %.2f GB", get_res_string(9307, 'shell32.dll'), mem_t['avail_gb'])
  local mem_s = str.format(mem_format, total_phys, avail_phys)
  set_text('mem_text', mem_s .. '    ' .. now_avail)

  local arch_id = 0
  if ARCH == 'x86' then
    arch_id = 56 -- Windows 10
    if vernum == "6.1" then arch_id = 23 end -- Windows 7
  else
    arch_id = 60 -- Windows 10
    if vernum == "6.1" then arch_id = 24 end -- Windows 7
  end
  set_res('arch_text', arch_id)

  set_res('host_group', 1545)
  set_res('host_label', 1547)
  set_res('fqdn_label', 1549)
  set_res('desc_label', 1551)
  set_res('workgroup_label', 1553)
  set_res('host_settings_btn', 1546)

  local computername = Reg:Read([[HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\ComputerName\ComputerName]]
        , 'ComputerName')

  set_text('host_text', computername)
  set_text('fqdn_text', computername)
  set_text('desc_text', '')
  local wg = os.getenv('USERDNSDOMAIN') or 'WorkGroup'
  set_text('workgroup_text', wg)

  set_text('status_group', 'Windows ' .. get_res_string(8981, 'shell32.dll'))

  set_text('runtime_label', 'System uptime')

  local t_format = get_res_string(28876, 'shell32.dll') .. ' ' .. get_res_string(28879, 'shell32.dll') .. ' ' .. get_res_string(28882, 'shell32.dll')
  if t_format and t_format:find("%%1!d!") then
    timetick_format = string.gsub(t_format, '%%1!d!', '%%02d')
  else
    timetick_format = '%02d:%02d:%02d'
  end
  runtime_text = sui:find('runtime_text')
  update_timetick()
  UI:SetTimer('System_Running_Tick', 1000)

  if os.info('locale') == 'zh-CN' then
    sui:find('edition_group').width = 85
    sui:find('system_group').width = 30
    sui:find('host_group').width = 148
    sui:find('status_group').width = 100
  end

  UIWindow.Inited = 1
end

UI.OnTimer['System_Running_Tick'] = function(id)
    update_timetick()
end

UI.OnClick['cphome_btn'] = function()
    App:Run('control')
end

UI.OnClick['devmgmt_btn'] = function()
    -- lua built-in function, but with cmd.exe's black window flushing
    --os.execute('start devmgmt.msc')
    App:Run('devmgmt.msc')
end

UI.OnClick['rmt_btn'] = function()
    App:Run('systempropertiesremote.exe')
end

UI.OnClick['protect_btn'] = function()
    App:Run('systempropertiesprotection.exe')
end

UI.OnClick['adv_btn'] = function()
    App:Run('systempropertiesadvanced.exe')
end

UI.OnClick['host_settings_btn'] = function()
    App:Run('systempropertiescomputername.exe')
end
