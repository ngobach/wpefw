# WinXShell UI Engine Reference (sui & suilib)

This document provides documentation for the undocumented **`sui`** (Shell UI) and **`suilib`** interfaces. These objects are injected by the C++ engine into the Lua runtime inside custom UI zip packages (such as `UI_Shutdown` or `UI_Volume`) to control layout elements, windows, and widget timers.

---

## 1. The `sui` Window Controller

`sui` represents the active DirectUI window frame. It exposes methods to query window telemetry, position the window, and find inner layout controls.

### Methods
* **`sui:find(name)`**
  Queries and returns a wrapper representing a layout element by its XML `name` attribute.
  * *Example:* `sui:find("$icon")` or `sui:find("restartbtn")`
  * *Returns:* A UI Control object (see section below).
* **`sui:moveto(x, y, width, height [, flag1 [, flag2]])`**
  Repositions and resizes the active window container.
  * *Example (Full screen resize):* `sui:moveto(0, 0, 2000, 2000, -1, 64)`
* **`sui:title(text)`**
  Sets the window header text.
  * *Example:* `sui:title("System Shutdown")`
* **`sui:info(query)`**
  Retrieves window geometry details.
  * `sui:info('rect')`: Returns coordinates/rectangle of the window.
  * `sui:info('wh')`: Returns window width and height.
* **`sui:click(ctrl)`**
  Programmatically triggers a click event action on the specified control element.
* **`sui:close()`**
  Closes and destroys the active UI window instance.

---

## 2. UI Control Object Properties

The object returned by `sui:find(name)` exposes get/set properties to dynamically manipulate XML layout controls.

| Property | Type | Description |
| :--- | :--- | :--- |
| **`.text`** | `string` | Gets or sets the label/button text content. |
| **`.bkimage`** | `string` | Sets the element background image path and attributes.<br>• *Format:* `"file='<path>' [blur='<val>'] [dest='x,y,w,h'] [fade='<alpha>']"`<br>• *Example:* `"file='blur.jpg' blur='5.0'"` |
| **`.visible`** | `boolean / int` | Toggles control visibility (`true` / `false` or `1` / `0`). |
| **`.selected`** | `int` | Sets the control selection highlight index (e.g. `1` or `0` for toggle items). |
| **`.enabled`** | `boolean` | Enables or disables mouse/keyboard interactions. |

---

## 3. The `suilib` Library

`suilib` is the low-level library injected by the DirectUI shell. Lua scripts use it for low-level window bindings and timers.

* **`suilib.print(...)`**
  Redirects Lua `print()` output inside the UI module to the debugger or shell logging output.
* **`suilib.call('SetTimer', timerid, interval)`**
  Initializes a timer bound to the UI window.
  * `timerid` *(integer)*: Unique ID for the timer.
  * `interval` *(integer)*: Duration in milliseconds.
* **`suilib.call('KillTimer', timerid)`**
  Terminates the UI timer matching `timerid`.
