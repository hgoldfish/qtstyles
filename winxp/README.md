# winxp — WinXPStyle (Office 2003 look)

Port of the upstream [WindowsModernStyle](http://doc.mimec.org/articles/wmstyle/index.html)
v1.1 (2009-11-23) by Michał Męciński — imitating the MS Office 2003 look for
toolbars, menus, docked windows and toolboxes — to Qt 5/6, renamed `winxp`.

* `winxpstyle.h/.cpp` — cross-platform port (BSD-licensed, see `COPYING`).
* `README.wmstyle`, `ChangeLog`, `COPYING` — original package files.

## Porting notes

The original style derived from `QWindowsVistaStyle` and called Windows APIs
(`uxtheme.dll`) to pick the active Luna/Aero color scheme, which made it
Windows-only and stuck on Qt 4. The port makes the following changes:

* The style now derives from **`QProxyStyle`** instead of
  `QWindowsVistaStyle`, so it works on any platform and any Qt 5/6 version.
  The base style is only used for the few widgets the style does not draw
  itself.
* **All common controls are drawn by the style itself.** The Windows XP
  "Luna" look — gradient faces, colored borders and the orange hover
  highlight — is reproduced with plain `QPainter` for all four color
  schemes: push buttons (gradient face, colored border and the orange
  default-button ring), tool buttons, check boxes, radio buttons, line
  edits, combo boxes, spin boxes, sliders, scroll bars, progress bars,
  group boxes, header views and every `QTabWidget` tab bar. The hover/press
  state changes are animated with a `QVariantAnimation` color ramp, so the
  XP rollover glow works on any platform without a Windows theme engine or
  `QWindowsVistaStyle`. Busy (indeterminate) progress bars are animated too:
  a `QTimer` (started only while such a bar is visible, tracked through a
  widget event filter) advances the bar's value so the green chunk travels
  back and forth along the groove, just like on Windows XP.
* `QProxyStyle` forwards internal sub-element calls (e.g. the base style
  drawing a checkbox indicator while handling `CE_CheckBox`) straight to the
  base style, bypassing the proxy overrides. The controls above therefore
  implement complete drawing pipelines (`CE_*`/`CC_*`/`PE_*`) that never
  fall back to the Win2000 look.
* The `uxtheme.dll` theme probing was dropped. The style now exposes the
  original four color schemes through the `Mode` constructor argument: the
  fixed Windows XP "Luna" palettes (Blue, Silver, Olive) and Classic, which
  derives its colors from the active `QPalette`.
* `layoutSpacingImplementation()` (a Qt 4-only protected hook) became an
  override of the public `layoutSpacing()`, and the legacy Qt 4 plugin
  machinery was replaced by the standard `QStylePlugin`/`Q_PLUGIN_METADATA`
  approach used by the other styles in this repository.

On Windows you can restore a native base look by assigning the platform style
as base, e.g.:

```cpp
WinXPStyle *style = new WinXPStyle(WinXPStyle::Blue);
style->setBaseStyle(QStyleFactory::create(QStringLiteral("windowsvista")));
qApp->setStyle(style);
```

## Standard palette

`standardPalette()` returns the classic Windows 2000 palette (beige surfaces
`#ece9d8`, navy selection `#000080`) as a **suggestion** — Qt never applies it
automatically, and Classic mode does not force it. The returned palette fills
in every `QPalette::ColorRole` (the Disabled group is derived by the shared
helper in `lib/qtstyles_palette.h`), so it is self-contained and never picks
up roles from the palette active at call time — switching styles at run time
cannot leak the previous style's colours. The three Luna variants are
the exception: Blue, Silver and Olive are fixed palettes applied through
`polish(QPalette&)` when the style is installed, so those keys always show
their matching XP scheme. Callers that want the Windows 2000 look with the
classic `winxp` key can apply it explicitly:

```cpp
qApp->setPalette(qApp->style()->standardPalette());
```

## Building

The plugin builds with both CMake (Qt 6) and qmake (Qt 5/6) from the
repository root; it installs to Qt's `styles` plugin directory and provides
four style keys: `winxp` (Classic, palette-based), `winxp-blue`,
`winxp-silver` and `winxp-olive` (fixed Windows XP Luna palettes).
