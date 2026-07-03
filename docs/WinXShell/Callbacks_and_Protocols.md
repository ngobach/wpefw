# WinXShell Callbacks and Custom Protocols

This document details the event callback hooks (such as `WxsHandler`, `Startmenu`, and `TrayClock`) and modern protocol handlers used to integrate WinXShell UI modules with system actions.

---

## 1. The `WxsHandler` Table

`WxsHandler` is a global bridging namespace registered directly by the C++ engine into the Lua runtime. Script configurations (e.g. inside `WinXShell.lua`) bind keys in this table to override standard shell behaviors.

| Field | Type | Description |
| :--- | :--- | :--- |
| `SystemProperty` | `string` | Behavior when clicking **"Properties"** on **"This PC"**:<br>• `'ui_systemInfo'`: Opens WinXShell's custom system info UI.<br>• `'system'`: Opens standard system properties (`sysdm.cpl`).<br>• `'auto'`: Selects custom info in WinPE; fallback to system properties on standard Windows.<br>• `nil` or `''`: Disables custom behavior. |
| `OpenContainingFolder` | `function` | Callback `function(lnkfile, realfile)` that handles the shortcut's context-menu click **"Open file location"**. Custom handlers can route this to third-party file managers. |
| `DisplayChangedHandler` | `function` | Event function triggered automatically when display resolution, monitor count, or DPI changes. |
| `TrayClockTextFormatter` | `function` | Periodic daemon handler to construct the tray clock display string (stores formatted string to the `ClockText` variable). |

### Background Integrations
WinXShell registers `WxsHandler` settings by writing directly to system registries at startup:
* **Properties Hook (`regist_system_property()`)**: Configures CLSID `{20D04FE0-3AEA-1069-A2D8-08002B30309D}` Shell Properties verb command target to launch `WinXShell.exe wxs-ui:systeminfo` or `wxs-open:system`.
* **Shortcut Location Hook (`regist_shortcut_ocf()`)**: Wires the "Open file location" shell context-menu command using `wxsStub.dll` (which routes callback arguments through `WxsHandler.OpenContainingFolder`).
* **UWP Protocol hooks (`regist_protocols()`)**: Maps the standard Windows protocols (`ms-settings`, `ms-availablenetworks`) to target the `WinXShell.exe` executable.

---

## 2. The `Startmenu` Event Interceptors

Start menu action items trigger specific object callbacks. Return values tell the engine whether to execute standard OS events.

* **`Startmenu:SetLogoId()`**
  Determines the Start Button taskbar logo resource. Returns an integer resource ID:
  * `0` = none
  * `1` = Windows flag
  * `2` = WinPE icon
  * `11-13` = Custom logo resources
* **`Startmenu:Logoff()`**
  Triggered when the user clicks the "Logoff" item. Return `1` to trigger the standard OS logoff command.
* **`Startmenu:Reboot()`**
  Triggered when the user clicks the classic Start Menu "Restart" button.
  * *Default behavior:* Calls `wxsUI('UI_Shutdown', 'full.jcfg')` and returns `0` (disabling standard OS reboot).
  * Return `1` to execute the default OS restart process immediately.
* **`Startmenu:Shutdown()`**
  Triggered when the user clicks the classic Start Menu "Shutdown" button.
  * *Default behavior:* Calls `wxsUI('UI_Shutdown', 'full.jcfg')` and returns `0` (disabling standard OS shutdown).
  * Return `1` to execute the default OS shutdown process immediately.
* **`Startmenu:ControlPanel()`**
  Triggered when the Control Panel item is clicked. Returns `0` (repress default) or `1` (run default).

---

## 3. The `TrayClock` Click Hooks

Enables customization of clicks on the system tray clock panel.

* **`TrayClock:onClick()`**
  Triggered when single-clicking the clock.
  * *Default behavior:* Opens the custom calendar module (`wxsUI('UI_Calendar', 'main.jcfg')`) and returns `0`.
* **`TrayClock:onDblClick()`**
  Triggered when double-clicking the clock.
  * *Default behavior:* Runs the Windows time configuration applet (`timedate.cpl`) and returns `0`.

---

## 4. Modern Protocol Handlers

WinXShell intercepts and processes UWP/modern system protocols using internal Lua mappings:

### `wxs-ui:` Protocol
Used to launch custom WinXShell UI modules:
* `wxs-ui:systeminfo` $\rightarrow$ launches `UI_SystemInfo.zip`
* `wxs-ui:settings` $\rightarrow$ launches `UI_Settings.zip` with parameter `-fixscreen`
* `wxs-ui:wifi` $\rightarrow$ launches `UI_WIFI.zip`
* `wxs-ui:volume` $\rightarrow$ launches `UI_Volume.zip`

### `wxs-open:` Protocol
Maps folder routes and opens specific Control Panel items:
* `wxs-open:controlpanel` $\rightarrow$ runs `control.exe`
* `wxs-open:printers` $\rightarrow$ displays device printers
* `wxs-open:networkconnections` $\rightarrow$ displays adapter properties

### `ms-settings:` Protocol
Redirects modern Windows 10/11 Settings links to classic/WinXShell components inside WinPE:
* `ms-settings:taskbar` $\rightarrow$ launches `UI_Settings` panel
* `ms-settings:display` $\rightarrow$ launches display/resolution settings panel
* `ms-settings:sound` $\rightarrow$ launches `UI_Volume` controls
* `ms-settings:network` $\rightarrow$ redirects to network connections folder
* `ms-settings:about` $\rightarrow$ launches custom system info
