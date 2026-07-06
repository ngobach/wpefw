# wxsUI Overview & Execution Reference

This document provides a comprehensive overview of the `wxsUI` framework inside WinXShell. `wxsUI` is a DirectUI (DuiLib-based) graphical user interface subsystem that allows developers to create lightweight, high-performance desktop interface widgets and dialogs for standard Windows and Windows PE environments using XML layouts and Lua scripting.

---

## 1. Directory & Package Structure

A `wxsUI` module is organized as a directory or packed into a `.zip` archive. The host application (`WinXShell.exe`) can load files directly from either format.

A standard `wxsUI` package contains the following structure:

```
wxsUI/UI_ModuleName/
├── main.jcfg            # Package configuration metadata (JSON format)
├── main.xml             # Main layout description (XML format)
├── main.lua             # Logic and window lifecycle events (Lua format)
├── locales/             # Text translation dictionaries and font maps
│   ├── en-US.xml
│   └── zh-CN.xml
├── themes/              # Color schemes and visual styling profiles
│   ├── default.xml
│   └── dark.xml
└── [assets]             # Directory or files containing images and icons (*.png, *.ico, *.jpg)
```

---

## 2. Window Configurations (`.jcfg`)

The `.jcfg` file (JSON configuration format) contains window setup properties that govern how the C++ host engine instantiates and behaves. 

### Configuration Properties Reference

| Property | Type | Description |
| :--- | :--- | :--- |
| **`name`** | `string` | **Required.** Specifies the unique identifier name of the UI module (e.g., `"UI_Volume"`). |
| **`title`** | `string` | Sets the default text header of the window (supports translations, e.g. `"%{Settings}"` or DLL resources, e.g. `"#{@systemcpl.dll,1}"`). |
| **`entry`** | `string` | Specifies the main entry layout XML file. Defaults to `"main.xml"`. |
| **`lua`** | `string` | Specifies the main Lua logic script file. Defaults to `"main.lua"` or matching base name. |
| **`class`** | `string` | Binds the window to a native C++ DirectUI class handler inside WinXShell. Known values:<br>• `CDUIVolumeWindow`: Volume panel integration.<br>• `CDUIWIFIWindow`: Network/Wi-Fi selection interface.<br>• `CDUICalendarWindow`: Clock/calendar desk overlay.<br>• `CDUISystemInfoWindow`: Diagnostic and hardware info. |
| **`singleton`** | `boolean` | If set to `true`, only one instance of the window is allowed to run. Re-opening will activate the existing window. |
| **`singleton_timer_interval`** | `integer` | Duration in milliseconds between checking singleton active states. |
| **`position`** | `string` | The default startup layout location on screen:<br>• `"center"`: Centered on screen.<br>• `"rightbottom"`: Mapped at bottom right (e.g. system tray flyout).<br>• `"leftbottom"`: Mapped at bottom left (e.g. start menu location).<br>• `"(auto)"`: Automatically positioned based on the parent panel context. |
| **`nobaricon`** | `boolean` | If set to `true`, hides the taskbar button window tab. |
| **`baricon`** | `string` | The path to the taskbar window icon (e.g. `"settings.ico"`). |
| **`customstyle`** | `boolean` | Toggles whether custom Win32 style/exstyle parameters are enforced. |
| **`style`** | `long integer` | Standard Win32 Window Styles flags (e.g., WS_POPUP, WS_VISIBLE). |
| **`exstyle`** | `long integer` | Extended Win32 Window Styles flags (e.g., WS_EX_TOOLWINDOW, WS_EX_TOPMOST). |
| **`theme`** | `string` | The visual theme folder/file to read stylesheet configs from (defaults to `"default"`). |
| **`locale`** | `string` | Forces the localization dictionary. Defaults to system language (e.g., `"en-US"`, `"zh-CN"`). |
| **`usesystemcaption`** | `boolean` | Set to `false` to disable the standard Windows window chrome frame, allowing layouts to draw custom title and caption bars. |
| **`minimizebox`** | `boolean` | Enables or disables the standard minimize button window capability. |
| **`maximizebox`** | `boolean` | Enables or disables the standard maximize button window capability. |
| **`startup_win`** | `string` | Set to `"max"` to open the window maximized at launch. |
| **`OnEscKey`** | `string` | Set to `"none"` to prevent the window from closing when pressing the `Esc` key. |
| **`OnDeactive`** | `string` | Event when window loses focus. Set to `"hide"` to automatically hide flyouts. |
| **`trans`** | `integer` | Sets window transparency opacity ranging from `0` (transparent) to `255` (opaque). |

> [!TIP]
> **Custom Parameters:** Developers can define arbitrary key-value pairs in `.jcfg` files (e.g., `"shadow": "Admin:admin"`, `"auto_logon_second": 5`, `"brightness": true`). These parameters are readable directly inside Lua script logic using `sui:jcfg('key')`.

---

## 3. Window Invocation & Command Line

WinXShell provides a CLI interface to launch UI panels directly from command lines, startup scripts, or background daemons.

### 1. Launching from Command Line
To run a custom UI package:
```bash
WinXShell.exe -ui -jcfg <path_to_ui> [arguments]
```

#### CLI Target Formats:
* **Directory Path:** Run from unpacked folders:
  ```bash
  WinXShell.exe -ui -jcfg wxsUI\UI_Shutdown\main.jcfg
  ```
* **Zip Archives:** Run directly from packed zip containers:
  ```bash
  WinXShell.exe -ui -jcfg wxsUI\UI_Shutdown.zip\full.jcfg
  ```

### 2. Custom Protocol Handlers (`wxs-ui:` and `wxs-open:`)
When running as a registered system handler, WinXShell intercepts system-wide URL protocols (which can be executed via `WinXShell.exe <protocol>` or standard Windows `ShellExecute`/`explorer.exe` triggers).

#### `wxs-ui:` Protocol
Launches registered custom `wxsUI` zip/folder modules directly:
* `WinXShell.exe wxs-ui:settings` $\rightarrow$ Launches the `UI_Settings` panel (equivalent to `wxsUI('UI_Settings', 'main.jcfg', '-fixscreen')`).
* `WinXShell.exe wxs-ui:wifi` $\rightarrow$ Launches the `UI_WIFI` panel.
* `WinXShell.exe wxs-ui:volume` $\rightarrow$ Launches the `UI_Volume` panel.
* `WinXShell.exe wxs-ui:systeminfo` $\rightarrow$ Launches the `UI_SystemInfo` panel.

#### `wxs-open:` Protocol
Routes generic configuration and folder open locations:
* `WinXShell.exe wxs-open:controlpanel` $\rightarrow$ Opens the system Control Panel.
* `WinXShell.exe wxs-open:netsettings` $\rightarrow$ Opens network connections properties.

### 3. Common Launch CLI Options
Many built-in `wxsUI` modules support parameter arguments parsed within their startup script loops:
* `-theme [light|dark]`: Overrides the theme style profile.
* `-bk <image_path>`: Sets a custom wallpaper background for Logon/Shutdown overlays.
* `-blur <float_value>`: Sets Gaussian blur radius level for overlays (e.g. `-blur 8.5`).
* `-user <username>`: Pre-selects the default logon account.
* `-direct`: Directly applies configurations without waiting for timer confirmation (e.g., screen resolution changes).
* `-nobeep`: Silences audio beep notifications.
* `-scroll`: Toggles ticker/scrolling animation effects for LED alerts.
* `-wait <seconds>`: Auto-terminates the widget panel after the elapsed duration.

### 4. Single UI Window Mode vs. Full Shell Mode

Passing or omitting the `-ui` (or `-jcfg`) flag changes the entire boot pipeline of the `WinXShell.exe` process:

* **Single UI Window Mode (With `-ui` or `-jcfg` flag)**:
  * WinXShell initializes only the `CDUIWindow` system via the `UIProcess()` entry point.
  * It bypasses the shell initialization (it does **not** draw the Desktop background, register taskbar/start menu panels, hook clock areas, or launch startup folder programs).
  * The process spawns only the single specified DirectUI layout window, runs its associated Lua scripting, and exits the process as soon as that window frame is closed.

* **Full Shell Mode / Daemon (Without `-ui` or `-jcfg` flag)**:
  * WinXShell boots into the main shell manager environment.
  * It draws the active desktop background (loading icons and arrangement options), renders the taskbar and clock, runs daemon listeners (clock clicks, hotkeys), and handles automatic startup program queues.
  * The process remains running persistently in the background.

### 5. Shell API Programmatic Execution
Within shell environments, you can launch custom UIs programmatically using the global `wxsUI` Lua API:
```lua
-- Syntaxes: wxsUI(ModuleName [, config_filename [, parameter_string]])
wxsUI('UI_Calendar', 'main.jcfg')
wxsUI('UI_Volume', 'main.jcfg', '-notrayicon -hidewindow')
```
