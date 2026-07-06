function onclick(ctrl)
  if ctrl == "cmd_item" then
    App:Run("cmd.exe")
    sui:hide()
  elseif ctrl == "notepad_item" then
    App:Run("notepad.exe")
    sui:hide()
  elseif ctrl == "explorer_item" then
    App:Run("explorer.exe")
    sui:hide()
  elseif ctrl == "taskmgr_item" then
    App:Run("taskmgr.exe")
    sui:hide()
  elseif ctrl == "settings_btn" then
    wxsUI("UI_Settings")
    sui:hide()
  elseif ctrl == "power_btn" then
    wxsUI("UI_Shutdown", "full.jcfg")
    sui:hide()
  end
end
