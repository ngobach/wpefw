
function onclick(ctrl)
  if ctrl == "restartbtn" then
    System:Reboot()
  elseif ctrl == "shutdownbtn" then
    System:Shutdown()
  end
end
