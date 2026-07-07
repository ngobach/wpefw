## Common guidelines

- This project uses Make.
- WinXShell wxsUI modules must be unpacked folders, not zip files.
- Use PECMD builtins instead of Windows standard command-line tools (WinPE lacks `taskkill`, `tskill`, etc.). For process management: `PECMD KILL <process>`, `PECMD EXEC <cmd>`.

## Practices

### Host-Guest communication via RPC broker

```bash
# Start broker
python3 scripts/rpc_broker.py

# Write a file to guest
python3 scripts/exec.py write_file "X:\\path\\file" /host/path/file

# Execute command on guest
python3 scripts/exec.py execute "command"

# Kill WinXShell (SHEL auto-restarts it)
python3 scripts/exec.py execute "PECMD KILL winxshell.exe"
```

### WinXShell hot reload

**Config changes** (`WinXShell.jcfg`):
1. Push updated file: `python3 scripts/exec.py write_file "X:\\WinXShell\\WinXShell.jcfg" src/wim-overlay/WinXShell/WinXShell.jcfg`
2. Restart: `python3 scripts/exec.py execute "PECMD KILL winxshell.exe"` — WinXShell is configured as `SHEL` in PECMD.ini, so it auto-restarts.

**wxsUI module changes**:
- Edit files directly in `src/wim-overlay/WinXShell/wxsUI/UI_<Module>/`
- Push live: `python3 scripts/exec.py write_file "X:\\WinXShell\\wxsUI\\UI_<Module>\\<file>" src/wim-overlay/WinXShell/wxsUI/UI_<Module>/<file>`
- UI module refreshes on next load; WinXShell restart not needed.

### Registry modifications on guest

Use `reg.exe`, not PECMD `REGI`:
```bash
reg.exe add "HKLM\..." /v "key" /t REG_SZ /d "value" /f
reg.exe add "HKLM\..." /v "key" /t REG_DWORD /d 1 /f
reg.exe query "HKLM\..." /v "key"
```

### Re-pushing files after VM restart

Guest filesystem is volatile (RAM-backed WinPE). Files pushed via `write_file` survive only until reboot. After restart, rebuild the overlay from `src/wim-overlay/` via the Makefile.
