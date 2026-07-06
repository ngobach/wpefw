# wxsUI Scripting & API Reference

WinXShell supports two scripting paradigms for writing user interface logic: a lightweight custom micro-script assembly language directly inside the `.jcfg` file, and standard full Lua files.

---

## 1. Custom Micro-Scripting (inside `.jcfg`)

For simple, lightweight dialog interactions (such as resolution changes or confirmation boxes), developers can write logic directly within the `.jcfg` JSON configuration without using a `.lua` file.

### Micro-Script Syntax & Symbols
* **`#`**: Refers to a UI layout control by its name attribute (e.g. `#resolution_combo`).
* **`$`**: Defines or reads a local scripting evaluation variable (e.g. `$last_resolution`).
* **`::`**: Invokes a host-injected action command (e.g. `::GetResolutionList`).
* **`=`**: Performs assignment. Syntax: `= <target>, <source>`
* **`PUSH`**: Pushes parameters onto the execution stack.
* **`CALL`**: Invokes the target method.

### Micro-Scripting Example
```json
{
  "name": "UI_Resolution",
  "OnLoad": "
    = #resolution_combo.list, ::GetResolutionList();
    = #resolution_combo.index, ::GetCurrentResolution();
    = $last_resolution, #resolution_combo.text;
    = #applybtn.enabled, FALSE;
  ",
  "OnClick": {
    "applybtn": "
      PUSH #resolution_combo.text;
      PUSH 1;
      CALL ::SetResolution();
      = #applybtn.visible,  FALSE;
      = #closebtn.visible,   TRUE;
    "
  }
}
```

---

## 2. Lua Scripting & Event Callbacks

For complex UI logic, the UI engine executes full Lua scripts. Scripts hook into window interactions using either **Global Script Functions** or **OOP-Style Class Tables**.

### 1. Global Callbacks
| Callback Function | Description |
| :--- | :--- |
| **`onload()`** | Executed immediately after the window and its layout controls are initialized. |
| **`onshow()`** | Executed whenever the window is set to visible. |
| **`onhide()`** | Executed whenever the window is hidden. |
| **`onclose()`** | Executed before the window frame is destroyed. |
| **`onclick(ctrl)`** | Fired when a control matching `ctrl` (string name) is clicked. |
| **`onchanged(ctrl, val)`** | Fired when a control value changes (e.g., slider drags, combo selections). |
| **`ontimer(id)`** | Fired periodically when a registered window timer ticks. |
| **`ondisplaychanged()`** | Fired when the display resolution, monitor count, or DPI changes. |
| **`onreturn(ctrl)`** | Fired when the user presses `Enter` inside an `<Edit>` input textbox control. |
| **`onmessage(msg, wparam, lparam)`** | Raw Win32 window message intercept handler. Returns `1` if handled. |

### 2. OOP-Style Event Callbacks
Alternatively, scripts can map callbacks onto the `UIWindow` and `UI` tables:
```lua
-- Lifecycle methods
function UIWindow:Init()
  self.Inited = 1
end

function UIWindow.OnLoad()
  UIWindow.Inited = 1
end

-- Event maps
UI.OnClick['restartbtn'] = function(ctrl)
  System:Reboot()
end

UI.OnChanged['brightness_slider'] = function(val, ctrl)
  Screen:Set('brightness', val)
end

UI.OnTimer['clock_tick'] = function(id)
  update_clock()
end
```

---

## 3. Host-Injected Lua APIs Reference

The C++ WinXShell engine injects several helper objects and libraries into the UI module's Lua context.

### 1. The `sui` Window Object
`sui` represents the active DirectUI window frame.

* **`sui:find(name)`**: Finds a layout control by its name. Returns a control wrapper.
* **`sui:moveto(x, y, w, h [, flag1, flag2])`**: Moves the window to absolute coordinates.
* **`sui:move(dx, dy, dw, dh)`**: Moves/resizes the window relative to its current coordinates.
* **`sui:title(text)`**: Sets the window title string.
* **`sui:info(query)`**: Retrieves geometric details. Queries:
  * `'rect'`: Returns window coordinates rect.
  * `'wh'`: Returns window width and height.
  * `'uipath'`: Returns the folder path of the active UI package.
* **`sui:click(ctrl)`**: Programmatically triggers click events.
* **`sui:close()`**: Destroys and closes the active window.
* **`sui:hide()`**: Hides the active window.
* **`sui:show()`**: Shows the active window.
* **`sui:focus(ctrl)`**: Grants keyboard focus to the control.
* **`sui:ishidden()`**: Returns `1` if the window is hidden, otherwise `0`.
* **`sui:jcfg(key)`**: Reads a key parameter defined in the active `.jcfg` file.
* **`sui:i18n(text)`**: Returns translated text based on the active locales dictionary.

#### Control Wrapper Properties:
The control wrapper returned by `sui:find(name)` exposes getter and setter properties:
* `.text` *(string)*: Label/button text content.
* `.bkimage` *(string)*: Slicing/image path string.
* `.visible` *(boolean/int)*: Toggles control visibility.
* `.selected` *(boolean/int)*: Toggles selection states.
* `.enabled` *(boolean)*: Toggles input interaction status.

---

### 2. The `suilib` Utility Library
`suilib` provides low-level window bindings.

* **`suilib.print(...)`**: Logs output messages.
* **`suilib.close()`**: Closes the active window.
* **`suilib.call('SetTimer', timerid, interval)`**: Registers a periodic UI timer (in ms).
* **`suilib.call('KillTimer', timerid)`**: Unregisters the UI timer.
* **`suilib.call('resstr', resource_string)`**: Queries resource string tables.
* **`suilib.call('run', path [, params])`**: Executes processes.
* **`suilib.call('GetTickCount')`**: Retrieves system uptime tick count in milliseconds.

---

### 3. Application level: `App`
* **`App.CmdLine`**: Raw CLI arguments.
* **`App:Print(value)`**: Prints messages.
* **`App:HasOption(option)`**: Returns `true` if option argument is present.
* **`App:GetOption(option)`**: Returns parameter data following option key.
* **`App:Run(exe [, params, showcmd])`**: Runs background processes.
* **`App:Exec(exe [, params])`**: Executes command synchronously (blocking).
* **`App:Sleep(ms)`**: Thread sleep duration.
* **`App:Call(action, ...)`**: Invokes internal API actions (e.g. `'exitcode'`).

---

### 4. Desktop Interface: `Desktop`
* **`Desktop:GetWallpaper()`**: Returns the path to the active wallpaper.
* **`Desktop:SetWallpaper(path)`**: Updates desktop wallpaper background.

---

### 5. System Settings: `System`
* **`System:Reboot()`**: Reboots computer.
* **`System:Shutdown()`**: Powers off computer.
* **`System:GetSetting(setting)`**: Reads theme variables (`'SysColorTheme'`, `'Colors.Transparency'`, `'ShellColorPrevalence'`).
* **`System:SetSetting(setting, val)`**: Writes theme options.
* **`System:SysColorTheme(val)`** / **`System:AppsColorTheme(val)`**: Configures system themes (`0` for light, `1` for dark).

---

### 6. Display Screen telemetry: `Screen`
* **`Screen:Get(param)`**: Queries display metrics (e.g., `'brightness'`, `'DPI'`).
* **`Screen:Set(param, val)`**: Writes metrics (e.g., brightness levels).
* **`Screen:GetX()`** / **`Screen:GetY()`**: Width and height boundaries of screen.

---

### 7. Registry manipulation: `Reg`
* **`Reg:Read(key, value)`**: Reads values.
* **`Reg:Write(key, value, data [, type])`**: Writes registry variables.

---

### 8. Windows Explorer Options: `FolderOptions`
* **`FolderOptions:Get(setting)`**: Queries folder options (e.g. `'ShowAll'`, `'ShowExt'`, `'ShowSuperHidden'`).
* **`FolderOptions:Set(setting, value)`**: Writes option changes.

---

### 9. Taskbar Interface: `Taskbar`
* **`Taskbar:GetSetting(setting)`**: Reads parameters (`'AutoHide'`, `'TaskbarSmallIcons'`).
* **`Taskbar:AutoHide(checked)`**: Toggles auto-hide taskbar functionality.
* **`Taskbar:UseSmallIcons(checked)`**: Toggles taskbar button dimensions.
* **`Taskbar:CombineButtons(mode)`**: Toggles grouping rules.

---

### 10. Audio Controller: `Volume`
* **`Volume:GetLevel()`**: Retrieves sound output levels (`0` - `100`).
* **`Volume:SetLevel(val)`**: Sets output volume level.
* **`Volume:IsMuted()`**: Returns `1` if volume is muted.
* **`Volume:Mute(val)`**: Toggles mute state.
* **`Volume:GetName()`**: Returns output audio device descriptor string.

---

### 11. File Helper: `File`
* **`File.Exists(path)`**: Returns `true` if file exists at target directory. Note the casing (`Exists` with capital E).
