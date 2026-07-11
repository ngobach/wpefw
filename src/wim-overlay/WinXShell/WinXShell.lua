App.ScriptEncoding = 'UTF-8'

is_pe = (os.info('isWinPE') == 1)
is_wes = App:HasOption('-wes')
is_win = App:HasOption('-windows')

function App:onLoad()
  WxsHandler.SystemProperty = 'ui_systemInfo'
  
  local last_res_x = 0
  WxsHandler.DisplayChangedHandler = function()
    local cur_res_x = Screen:GetX()
    if last_res_x == cur_res_x then return end
    last_res_x = cur_res_x
    if last_res_x >= 3840 then
      Screen:DPI(150)
    elseif last_res_x >= 1440 then
      Screen:DPI(125)
    elseif last_res_x >= 800 then
      Screen:DPI(100)
    end
  end

  WxsHandler.OpenContainingFolder = function(lnkfile, realfile)
    local parent_dir = realfile:match("(.+)\\")
    if parent_dir then
      App:Run("X:\\Program Files\\Explorer++\\Explorer++.exe", '"' .. parent_dir .. '"')
    end
  end
end

function App:onShell()
  -- wxsUI('UI_Volume', 'main.jcfg', '-hidewindow')
  -- wxsUI('UI_Wifi', 'main.jcfg', '-hidewindow')
end

function Startmenu:Shutdown()
  wxsUI('UI_Shutdown')
end

function StartButton:onClick()
  App:Run("pestart.exe")
end
