local power_clicked = false

function onload()
    local wp = ''
    local param = App.CmdLine
    local blur = string.match(param, '-blur%s([0-9\\.]+)%s?')
    if blur == nil then blur = '0.0' end

    wp = string.match(param, '-bk%s(.+)$')
    if wp == nil then wp = Desktop:GetWallpaper() end

    local win = sui:find('full_win')
    win.bkimage = "file='" .. wp .. "' blur='" .. blur .. "'"
    sui:moveto(0, 0, 2000, 2000, -1, 64)
end

function onclick(ctrl)
  if power_clicked then return end
  if ctrl == "restartbtn" then
    power_clicked = true
    System:Reboot()
  elseif ctrl == "shutdownbtn" then
    power_clicked = true
    System:Shutdown()
  end
end
