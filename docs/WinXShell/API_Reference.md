# WinXShell Lua API Reference

This document provides a comprehensive reference for the WinXShell Lua scripting interface, detailing the global tables, namespaces, utility extensions, properties, and methods that are exposed to scripts running in the shell environment.

---

## 1. Global Objects & Utilities

### Core Functions
* **`wxsUI(ui [, jcfg [, param]])`**
  Loads and launches a custom UI package (usually zipped under the `wxsUI/` directory).
  * `ui` *(string)*: Name of the UI module (e.g. `'UI_Shutdown'`, `'UI_Volume'`). If `<ui>.zip` exists, it automatically appends `.zip`.
  * `jcfg` *(string, optional)*: Startup configuration file. Defaults to `'main.jcfg'`.
  * `param` *(string, optional)*: Additional parameters to pass to the window wrapper.
* **`CloseShellWindow()`**
  Instructs the host shell application to close and terminate the environment.
* **`ShellDaemon(wait, cmd)`**
  Runs a child process wrapper inside a monitoring loop.

### Window Helper Wrappers
* **`MinimizeWindow(class, title)`**
  Minimizes a window matching the specified Win32 class and/or window title.
* **`HideWindow(class, title)`**
  Hides a window matching the specified Win32 class and/or window title.
* **`CloseWindow(class, title)`**
  Closes a window matching the specified Win32 class and/or window title.

### String & OS Extensions
* **`string.envstr(str)`**
  Resolves environment variables inside strings (e.g. `"%SystemRoot%"` becomes `"X:\Windows"`).
* **`string.resstr(str)`**
  Extracts localized text entries from executable and library PE resource wrappers.
* **`math.band(a, b)`**
  Performs a bitwise AND operation on two integers.
* **`os.putenv(var, value)`**
  Registers or overrides environment variables inside the current session.
* **`os.exists(path)`**
  Returns `true` if the specified file or folder exists.

---

## 2. The `App` Object

The `App` object exposes application-level descriptors, flags, timers, and basic execution helpers.

### Properties
| Property | Type | Description |
| :--- | :--- | :--- |
| `App.Name` | `string` | The executable name (`WinXShell.exe`). |
| `App.Path` | `string` | Path to the folder containing the executable. |
| `App.FullPath` | `string` | Absolute path to the running executable. |
| `App.CmdLine` | `string` | Raw command line parameters passed to the application. |
| `App.Version` | `string` | Engine version string. |
| `Lua.Version` | `string` | Embedded Lua interpreter version (e.g. `5.4`). |

### Methods
* **`App:Run(executable [, parameters])`**
  Spawns a process in the background.
* **`App:Exec(executable [, parameters])`**
  Executes a command and blocks until the process finishes.
* **`App:Sleep(milliseconds)`**
  Suspends script execution for a specified duration.
* **`App:HasOption(option)`**
  Returns `true` if the specified option (e.g. `"-wes"`) is present in the command line.
* **`App:GetOption(option)`**
  Retrieves parameters associated with the specified command line option.
* **`App:SetTimer(id, interval)`**
  Starts a periodic execution timer. Fires `App:onTimer(id)` when the interval expires.
* **`App:KillTimer(id)`**
  Stops the periodic timer.
* **`App:JCfg(section, key1 [, key2, ...])`**
  Reads a nested configuration value from the active `.jcfg` file.

---

## 3. The `Shell` Object

The `Shell` object controls the desktop area, wallpaper, desktop icons, and taskbar pinning.

### Desktop Management
* **`Desktop:Refresh()`**
  Forces the system to redraw and refresh desktop icons.
* **`Desktop:SetWallpaper(path)`**
  Sets the active desktop background image.
* **`Desktop:GetWallpaper()`**
  Reads the path of the current desktop wallpaper image.
* **`Desktop:SetIconSize(level)`**
  Sets the size level of desktop icons.
* **`Desktop:ShowIcons(checked)`**
  Toggles the visibility of desktop icons (`true` to show, `false` to hide).
* **`Desktop:AutoArrange(checked)`**
  Toggles whether desktop icons are automatically arranged.
* **`Desktop:SnapToGrid(checked)`**
  Toggles whether desktop icons snap to the layout grid.
* **`Desktop:Link(lnk, target [, param [, icon [, index [, showcmd]]]]))`**
  Creates a shortcut file (`.lnk`) on the user's Desktop.

### Taskbar & Start Menu Pinned Items
* **`Taskbar:AutoHide(checked)`**
  Toggles auto-hiding behavior of the taskbar.
* **`Taskbar:Hide()`** / **`Taskbar:Show()`**
  Hides or shows the taskbar panel.
* **`Taskbar:Pin(target [, name [, param [, icon [, index [, showcmd]]]]])`**
  Pins a program shortcut to the Taskbar Quick Launch area.
* **`Startmenu:Pin(target [, name [, param [, icon [, index [, showcmd]]]]])`**
  Pins a program shortcut to the Start Menu workspace.
* **`Startmenu:Link(lnk, target [, param [, icon [, index [, showcmd]]]])`**
  Creates a shortcut file (`.lnk`) in the Start Menu directory.

---

## 4. The `System` Object

Exposes system actions, hardware telemetry, display properties, registry wrappers, and dialogue prompts.

### System Actions
* **`System:Reboot()`**
  Reboots the machine.
* **`System:Shutdown()`**
  Shuts down the machine.
* **`System:SysColorTheme(mode)`** / **`System:AppsColorTheme(mode)`**
  Sets light (`0`) or dark (`1`) themes for shell components and applications.
* **`System:CreatePageFile(file, min, max)`**
  Initializes a Windows virtual memory pagefile.

### Registry (`Reg`)
* **`Reg:Read(key, value)`**
  Reads a value from the registry. Returns `nil` if not found.
* **`Reg:Write(key, value, data [, type])`**
  Writes a registry value. `type` defaults to `REG_SZ`.
* **`Reg:GetSubKeys(key)`**
  Returns a list table containing all subkeys under a registry key path.
* **`Reg:Delete(key [, value])`**
  Deletes a registry value or an empty subkey.
* **`Reg:DeleteKey(key)`**
  Recursively deletes a registry key and all of its subkeys.

### Screen Display (`Screen`)
* **`Screen:GetX()`** / **`Screen:GetY()`**
  Retrieves screen resolution width and height.
* **`Screen:GetDPI()`** / **`Screen:DPI(scale)`**
  Retrieves or updates the display DPI scaling percentage.
* **`Screen:Disp(width, height)`**
  Sets screen resolution size.

### Native Dialogs (`Dialog`)
* **`Dialog:Show(caption, message [, buttons [, icon]])`**
  Launches a standard Win32 Message Box.
* **`Dialog:OpenFile(title, filters [, initDir])`**
  Pops up an Open File Picker.
* **`Dialog:BrowseFolder(title [, csidl])`**
  Pops up a Directory browser folder picker.

---

## 5. Window & Process Interface (`Proc`)

Fired by calling `Window.Find(title, class)` or `Window.Match(pattern)`. Returns a `Proc` object wrapping an active window handle.

### Methods
* **`:GetClassName()`** / **`:GetFileName()`**
  Returns the window class name or the path of the process owner executable.
* **`:IsVisible()`**
  Returns `true` if the window is currently visible.
* **`:Show(flag)`**
  Sets window visibility using standard Win32 flags (e.g. `SW_HIDE=0`, `SW_SHOW=5`).
* **`:Minimize()`** / **`:Maximize()`** / **`:Activate()`** / **`:Hide()`**
  Changes window state.
* **`:Close()`** / **`:Quit()`**
  Sends standard close messages (`WM_CLOSE`, `WM_QUIT`) to the window.
* **`:Kill()`**
  Terminates the process owner immediately.
* **`:SendMessage(msg, wparam, lparam)`** / **`:PostWindow(msg, wparam, lparam)`**
  Dispatches low-level Win32 messages to the window hook.
