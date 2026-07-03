# WinXShell Configuration Schema (WinXShell.jcfg)

This document details the configuration namespaces and properties supported in `WinXShell.jcfg` (or `main.jcfg`), which governs the visual appearance, themes, start menu, desktop, and taskbar items.

---

## 1. `JS_DESKTOP` (Desktop Setup)

Configures the workspace area, wallpaper image, icon scaling, and context-menu hooks.

* **`wallpaper`** *(string)*: Path to the background image. Supports environment macros (e.g. `"%windir%\\web\\wallpaper\\Windows\\img0.jpg"`).
* **`wallpaperstyle`** *(integer)*: Layout format for the wallpaper image:
  * `0` = Stretch (Default)
  * `1` = Tile
  * `2` = Center
* **`iconsize`** *(integer)*: Size level in pixels for desktop icons (e.g. `32`, `48`).
* **`cascademenu`** *(object)*: Binds custom context sub-menus to right-clicks on the desktop.
* **`3rd_open_arguments`** *(string)*: Format string parameters passed when double-clicking directory folders.

---

## 2. `JS_THEMES` (Visual Profiles)

Defines named color schemes and transparency styles. The taskbar and shell elements map their layouts to the active theme profile.

### Theme Fields:
* **`taskbar`** *(object)*:
  * **`bkcolor`** *(array)*: Background color defined as `[Red, Green, Blue]` values (e.g. `[0, 120, 215]`).
  * **`bkmode`** *(string)*: Transparency mode of the panel:
    * `"opaque"`: Solid background color.
    * `"transparent"`: Basic transparency.
    * `"acrylic"`: Windows Acrylic blur filter.
  * **`transparency`** *(integer)*: Opacity percentage value ranging from `0` (invisible) to `100` (opaque).
  * **`task_line_color`** *(array)*: Accent underline highlight color for active taskbar window button tabs.

---

## 3. `JS_TASKBAR` (Taskbar Setup)

Controls size, theme matching, and visible features on the taskbar.

* **`theme`** *(string)*: Name of the visual theme to load from the `JS_THEMES` definitions (e.g. `"dark"`).
* **`height`** *(integer)*: Panel height in pixels (defaults to `40`).
* **`smallicon`** *(boolean)*: Toggles small size scaling for taskbar task buttons.
* **`thumbnail`** *(boolean)*: Enables window task hovering thumbnails.
* **`task_close_button`** *(boolean)*: Displays an inline close button directly inside taskbar items.
* **`no_task_title`** *(boolean)*: Hides text descriptions next to task button icons.
* **`userebar`** *(boolean)*: Toggles rebar container alignment.

---

## 4. `JS_STARTMENU` (Start Button & Menu)

Controls taskbar Start Button logo properties and customize action commands.

* **`start_pushed_bkcolor`** *(array)*: Hover/Pressed color accent for the Start Button.
* **`start_icon`** *(string)*: Icon graphic scheme (`"theme"`, `"empty"`, or a file path to a custom `.ico` file).
* **`start_command`** *(string)*: Custom callback function triggered when single-clicking the Start Button.
* **`noshutdown`** / **`norestart`** / **`nologoff`** / **`nofind`** / **`norun`** *(boolean)*: Toggles visibility of respective button links inside the start menu.
* **`commands`** *(object)*: Redefines system actions when clicking Restart or Shutdown inside the menu list:
  * **`reboot`** / **`shutdown`** *(object)*:
    * `command` *(string)*: Process path to run (e.g. `X:\Windows\System32\PECMD.exe` or `WinXShell.exe`).
    * `parameters` *(string)*: Arguments passed to the target command.

---

## 5. `JS_QUICKLAUNCH` (Quick Launch Pinned Items)

Configures layout and directory bindings for taskbar pinned shortcuts.

* **`folder`** *(string)*: Directory path containing shortcut `.lnk` files to pin to the taskbar.
* **`maxiconsinrow`** *(integer)*: Maximum allowed pinned icons before collapsing.
* **`hide_showdesktop`** *(boolean)*: Hides the right-most "Show Desktop" click bar.
* **`hide_fileexplorer`** *(boolean)*: Hides the default folder explorer shortcut.

---

## 6. `JS_NOTIFYAREA` (System Tray)

Toggles whether WinXShell overlays native Win32 system tray components.

* **`handle_system_network`** *(boolean)*: Toggles custom volume and adapter flyout modules.
* **`handle_system_volume`** *(boolean)*: Toggles custom system sound control overlays.
* **`hide_toggle_button`** *(boolean)*: Hides the taskbar notification tray collapse overflow arrow.

---

## 7. `JS_DAEMON` (Agent Listeners)

* **`clockarea_click`** / **`clockarea_dbclick`** *(object)*: Executable command and parameter arguments to launch when clicking the taskbar clock.
* **`handle_CAPS_double`** *(boolean)*: Listens for double-presses of the `Caps Lock` key to run custom callback tasks.

---

## 8. `JS_FILEEXPLORER` (Alternative File Explorer)

* **`3rd_filename`** *(string)*: Process path of an alternative file manager (such as Explorer++).
* **`3rd_startup_arguments`** *(string)*: Default startup arguments when opening directories.
* **`WIN+E`** *(object)*: Custom execution command target triggered when pressing the `Win + E` keyboard shortcut.
