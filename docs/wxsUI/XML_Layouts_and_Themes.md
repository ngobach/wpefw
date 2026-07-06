# wxsUI XML Layouts & Themes Reference

This document provides a detailed reference of the XML DirectUI (DuiLib-based) layout engine, controls, properties, styling system, and theme mechanisms supported by `wxsUI`.

---

## 1. Window Layout Controls & Elements

Layout layouts are defined hierarchically. Containers structure layout alignment (vertical/horizontal flow), while controls process text rendering, inputs, images, and click triggers.

### Root Container: `<Window>`
Every layout starts with a `<Window>` root element.
* **Attributes:**
  * `size="width,height"`: Default startup dimensions (e.g. `size="800,600"`).
  * `mininfo="width,height"`: Minimum allowed resize dimensions.
  * `caption="left,top,right,bottom"`: Defines the drag-to-move caption window area. Usually set to `0,0,0,32` to make the top 32px of the window act as a draggable caption bar.
  * `sizebox="left,top,right,bottom"`: Resizable border border-width (e.g. `sizebox="6,6,6,6"`).
  * `defaultfontcolor="#ARGB"`: Default text hex color.

### Core Layout Containers
* **`<VerticalLayout>` / `<HorizontalLayout>`**: Organizes child nodes sequentially in columns or rows.
  * *Attributes:* `height`, `width`, `padding`, `childpadding`, `bkcolor`, `bkimage`, `bordersize`, `bordercolor`, `borderround`, `vscrollbar`, `hscrollbar`.
* **`<TabLayout>`**: Holds a stack of tab panels where only one panel is visible at a time.
  * *Attributes:* `name`, `selectedid`, `padding`.

### Core Widgets & Interactive Controls
* **`<Control>`**: Acts as an empty spacer or structural divider.
* **`<Label>`**: Renders static text or icons.
  * *Attributes:* `text`, `font`, `textcolor`, `textpadding`, `align`, `bkimage`, `showhtml`.
* **`<Text>`**: Text element supporting HTML-like styles (e.g. `<a>` links and formatting).
* **`<Button>`**: Triggers click handlers.
  * *Attributes:* `name`, `text`, `font`, `textcolor`, `hottextcolor`, `bkimage`, `hotimage`, `normalimage`, `enabled`, `visible`, `shortcut`, `showhtml`, `style`, `borderround`, `bordersize`, `bordercolor`, `hotbordercolor`, `focusbordercolor`.
* **`<Combo>`**: Renders dropdown boxes. Contains child `<ListLabelElement>` elements.
* **`<Slider>`**: Renders range sliders.
  * *Attributes:* `name`, `imm="true"` (fires immediate value change event), `min`, `max`, `value`, `thumbsize`, `thumbimage`, `bkimage`, `foreimage`.
* **`<Option>`**: Acts as a toggle switch or radio button.
  * *Attributes:* `name`, `selected="true|false"`.
* **`<Edit>`**: Renders single-line text input fields.
  * *Attributes:* `name`, `text`, `textcolor`, `bkcolor`, `nativebkcolor`, `align`, `password="true|false"`, `focusedimage`.
* **`<Treeview>`**: Hierarchical tree viewer. Contains child `<TreeNode>` elements.
* **`<TreeNode>`**: Element node within a `<Treeview>` element.
  * *Attributes:* `text`, `visible`, `folderattr`, `itemattr` (custom attribute list).

---

## 2. Shared Styles & Themes

`wxsUI` supports global styles and themes to decouple appearance rules from individual layout elements.

### Color Themes (`themes\*.xml`)
Themes are structured XML files containing `<ColorTheme>` root tags and reusable `<Style>` styles:
```xml
<?xml version="1.0" encoding="utf-8"?>
<ColorTheme>
    <Style name="ct-bktitle" bkcolor="#FF303F9F" />
    <Style name="ct-bksubtitle" bkcolor="#FF3F51B5" />
    <Style name="attentionBtn" width="120" height="32" textcolor="#FF000000" hottextcolor="#FFFFFF" hotimage="color='#FFFF4081'" />
</ColorTheme>
```

### Style Templating & Parameterization
`wxsUI` styles support a template replacement mechanism. Style templates specify parameters using `$1`, `$2`, etc. when writing values:
```xml
<!-- Style template definition in themes/default.xml -->
<Style name="PowerBtn" value="width='128' bkimage='file=&quot;$1B.png&quot;' hotimage='file=&quot;$1.png&quot;'" />

<!-- Referencing style with parameter value -->
<Button name="restartbtn" text="Restart" style="PowerBtn:reboot" />
```
During rendering, the engine replaces `$1` with the suffix specified after the colon (`reboot`), resolving the image paths to `rebootB.png` and `reboot.png`.

---

## 3. Localization & Multi-Language (`locales\*.xml`)

Languages are configured in `locales` dictionary files containing translation definitions and font mappings:
```xml
<?xml version="1.0" encoding="utf-8"?>
<Locale>
    <!-- Font mapping overrides for this locale -->
    <Font shared="true" id="16" size="16" name="Segoe UI" default="true" />
    <Font shared="true" id="sym" size="24" name="Segoe MDL2 Assets" />

    <!-- Translation keys -->
    <MultiLanguage id="SignIn" value="Sign In" />
    <MultiLanguage id="Cancel" value="Cancel" />
</Locale>
```

### Referencing Translations
* **In XML Layouts:** Reference translation string keys using the `%{string_id}` syntax:
  ```xml
  <Button name="cancelbtn" text="%{Cancel}" />
  ```
* **In Lua Scripts:** Retrieve translated strings dynamically using the `sui:i18n()` api:
  ```lua
  local msg = sui:i18n('will autologon with the $user account')
  ```

---

## 4. Advanced XML Resources & Tricks

### 1. Extracting System Strings (`#{@dll_name,resource_id}`)
WinXShell can read localized text strings directly from system DLL resources.
* **Syntax:** `#{@<dll_name>,<resource_id>}`
* **Example:**
  ```xml
  <!-- Reads string index 1 from systemcpl.dll -->
  <Label text="#{@systemcpl.dll,1}" />
  ```

### 2. Extracting Icons from Files & DLLs
Background and control image fields (`bkimage`, `hotimage`, `normalimage`) support extracting embedded icon frames from files.
* **Syntax Options:**
  * **File icon association (`file='...,F'`):** Appending `,F` instructs the engine to load the file's system associated file icon:
    ```xml
    <TreeNode text="Paint" itemattr="bkimage=&quot;file='mspaint.exe,F'&quot;" />
    ```
  * **Explicit index (`,index`):** Loads the icon by its 0-based frame index:
    ```xml
    <Button normalimage="file='shell32.dll,2'" />
    ```
  * **Explicit Resource ID (`,-resource_id`):** Loads the icon matching a specific negative Windows PE resource identifier:
    ```xml
    <Button bkimage="file='shell32.dll,-167'" />
    ```

### 3. Image Property Attributes
The `bkimage`, `normalimage`, and `hotimage` attributes support sub-property option values wrapped inside single quotes:
* **`file`**: Path to the asset.
* **`dest`**: Target rect alignment bounding box: `dest='x,y,width,height'` (e.g. `dest='32,32,64,64'`).
* **`corner`**: Border slicing layout configuration for 9-slice image scaling: `corner='left,top,right,bottom'` (e.g. `corner='2,2,32,2'`).
* **`fade`**: Opacity transparency alpha scale value (from `0` to `255`).
* **`blur`**: Applies Gaussian blur filtering radius at runtime (e.g. `blur='10.0'`).
* **`color`**: Sets solid background color fillers (e.g., `color='#FFF48FB1'`).

---

## 5. Dynamic Layout Injection

Developers can inject dynamic XML structures directly into container layouts at runtime from Lua using the `:add()` layout insertion method.
* **Syntax:** The inserted string must be wrapped inside a `<elem>...</elem>` tag block.
* **Example:**
  ```lua
  local container = sui:find('left_panel')
  container:add([[
    <elem>
      <Treeview name="app_tree" width="300" vscrollbar="true">
        <TreeNode text="Accessories" />
      </Treeview>
    </elem>
  ]])
  ```

---

## 6. C++ DirectUI (DuiLib) Architecture Notes

WinXShell parses and renders UI layouts using **DuiLib**, a well-known open-source DirectUI library in C++.

### Engine Compilation & Linkage
The host application links against DuiLib configurations:
* `DuiLib_u.lib` / `DuiLib_u_x64.lib`: Release unicode configurations.
* `DuiLib_ud.lib` / `DuiLib_ud_x64.lib`: Debug unicode configurations.

### `CDUIWindow` & C++ Wrappers
Window instances loaded via the `-ui` CLI parameter are managed by `CDUIWindow` (a C++ base class that wraps DuiLib's `CWindowWnd` and handles frame operations).
* **Control Binding:** DirectUI elements are parsed and managed by the `CPaintManagerUI` resource manager, which is set to the resource path directory (e.g. `wxsUI\UI_Settings\`).
* **Interpreter Integration:** C++ uses the `SUIInterpreter` engine to parse micro-scripts inside JCFG files. In open-source forks of the shell (such as Sandboxie-Plus `SbieShell`), the C++ `DUI/` source files and precompiled libraries are sometimes excluded or kept private, requiring the standard DuiLib library for manual rebuilds.

