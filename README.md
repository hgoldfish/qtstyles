modified the original qt style plugins to improve the quality under hidpi environment.

* oldschool  ==  motif: the classic Motif drawing and metrics straight from
  `QMotifStyle` (bevels, default-button indicator, tristate check boxes,
  inverted selection, Motif arrows and scroll bars).  Pure Motif look, no
  SGI colouring.
* newschool  ==  cde
* highschool ==  OldschoolStyle with the SGI / Irix accents layered on top:
  the warm beige-grey palette (beige input wells, Motif inverted selection)
  and the red SGI check marks / red radio dots in diamond indicators.  Every
  other widget keeps the classic Motif drawing and metrics.  A clean-room
  rewrite; shares no code with the original Qt 3 SGI style by Trolltech.
* plastic    ==  plastique
* dirtylooks ==  cleanlooks

* winxp      ==  WindowsModernStyle (Office 2003 look, upstream
  [wmstyle](http://doc.mimec.org/articles/wmstyle/index.html)). It was ported
  from the Qt 4 `QWindowsVistaStyle`-based original to a `QProxyStyle`-based
  cross-platform implementation. The style draws the common controls itself
  with a Windows XP "Luna" appearance using plain `QPainter` — push buttons,
  tool buttons, check boxes, radio buttons, line edits, combo boxes, spin
  boxes, sliders, scroll bars, progress bars, group boxes, header views and
  all `QTabWidget` tab bars — including the hover/press transition
  animation and the animated busy (indeterminate) progress bars, so they
  work without a Windows theme engine. The plugin also
  ships `winxp-blue`, `winxp-silver` and `winxp-olive` with the fixed
  Windows XP Luna palettes.
* phase      ==  phase (original Qt 4/KDE widget style by David Johnson, MIT).
  Like the other plugins it derives from `QProxyStyle`: layout and the widgets
  it does not re-draw are delegated to the **Windows** base style
  (`QStyleFactory::create("Windows")`, a `QWindowsStyle`), matching the
  original Qt 4 style which inherited `QWindowsStyle`. In Qt 6 that class is
  no longer part of the public API, but the runtime style is still reachable
  through the factory key and keeps the classic metrics/look the original
  relied on. `QCommonStyle` alone yields broken metrics (e.g. menu-bar items
  with no padding at all, a zero-width splitter handle), hence the proxy.

The three styles below are **clean-room rewrites**: the look was specified from
the appearance of the originals (used as visual references only), but no code
from them or from later community ports is included. They are all covered by
the project's LGPL license.

* bluecurve ==  the default widget theme of Red Hat 8/9 (2002). A `QCommonStyle`
  with palette-derived shade ramps: light beveled buttons with a 1-pixel
  highlight rim, sunken line-edit wells, blue gradient highlights for selected
  items and progress bars, and 13x13 checkbox/radio indicators.
* platinum ==  the classic Mac OS 8/9 "Platinum" look from Qt 3
  (`QPlatinumStyle`). Built on `QProxyStyle` with the `windows` base style:
  square command buttons with cut corners, multi-line beveled panels,
  13x13 checkbox / radio indicators, 7-pixel slider troughs with hexagonal
  handles and riffle-decorated scroll-bar buttons.
* keramik  ==  the ceramic-themed KDE 3 / TDE style (2002). Built on
  `QProxyStyle`: smooth bevel gradients, rounded buttons with thin frames,
  sunken rounded input wells, and palette-highlight slider / scroll-bar
  handles. Besides the classic widgets it also covers the modern control set —
  tool buttons with menu indicators, group boxes, tool boxes, dock-widget
  titles, tool tips, tree branches, size grips and animated busy progress
  bars.

## Colour scheme policy

Every style provides a classic colour palette through
`QStyle::standardPalette()` — the palette the original desktop environment it
imitates used (e.g. the SGI beige-grey of `highschool`, the Mac "Platinum"
warm grey of `platinum`, or the Windows 2000 palette of `winxp`). This is a
**suggestion only**: Qt never applies it automatically (the platform-theme
palette wins), and none of the plain style keys forces it. Applications are
free to call `QApplication::setPalette(style->standardPalette())` when they
want the classic look, or to keep their own palette.

Every `standardPalette()` fills in the full role set explicitly for **all three
`QPalette::ColorGroup`s** (Active, Inactive and Disabled) — every `ColorRole`
the widgets can use: the text/selection roles, the bevel ramp
(Light/Midlight/Mid/Dark/Shadow) that buttons, frames, sliders and scroll bars
draw with, the tool-tip and placeholder roles, link colours, and on Qt 6.8+ the
`Accent` role (aliased to `Highlight`). The Disabled group is derived by the
shared helper in `shared/qtstyles_palette.h`, so the returned palette is
**self-contained**: it does not inherit from the palette active at call time.
This matters when switching styles at run time — an incomplete palette would
silently pick up roles from the previously applied palette.

Each style also ships a `-classic` variant (`keramik-classic`,
`oldschool-classic`, `newschool-classic`, `highschool-classic`,
`plastic-classic`, `phase-classic`, `winxp-classic`, `dirtylooks-classic`,
`bluecurve-classic`, `platinum-classic`). Unlike the plain keys, which leave
the incoming palette alone, selecting one of these keys **forces** the style's
`standardPalette()`: it is applied through `polish(QPalette&)` when the style
is installed, so the classic look always wins regardless of the host theme.
The same mechanism is used by the three fixed Luna variants `winxp-blue`,
`winxp-silver` and `winxp-olive`, whose hard-coded Windows XP colours are
applied at installation time.

## Building for Qt 6

The project builds with CMake against Qt 6:

```sh
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt6 -j$(nproc)
cmake --install build-qt6
```

Notes:

* Qt 6.6 and newer is required (a C++17-capable compiler, e.g. GCC 10+).
* The `plastic` plugin reads a platform theme hint through Qt private headers,
  so it links `Qt6::GuiPrivate`; install the matching private-headers package
  for your distribution if `qpa/qplatformtheme.h` is not found.
* The plugins install to Qt's `styles` plugin directory, e.g.
  `<prefix>/lib64/qt6/plugins/styles`.

The legacy qmake build still targets Qt 5 and uses the C++11 standard, so the
sources must stay compatible with C++11 (no C++14/17-only constructs).

## Building for Qt 5 (qmake)

```sh
qmake-qt5 ..        # or just: qmake ..  (in a build directory)
make -j$(nproc)
```

The build must be out-of-source (as above). Do not run qmake inside the source
tree — leftover in-source moc artifacts (`.moc/`, `Makefile`, …) confuse qmake
and break the `#include "plugin.moc"` plugin sources.

## Preview program

`preview/` is a Qt application that previews these styles on a window packed
with many Qt widgets (buttons, inputs, lists/trees/tables, progress bars,
sliders, tabs, toolboxes, text editors, frames, calendars, standard dialogs,
menus, toolbars, docks …). The styles are compiled in directly, so it works
without installing the plugins first. It builds with both Qt 5 (qmake) and
Qt 6 (CMake or qmake):

```sh
cmake --build build-qt6 --target preview
./build-qt6/preview/preview --style phase
```

Switch styles live from the **Style:** drop-down in the toolbar. See
`preview/README.md` for details.

## High-DPI support

Qt 6 scales high-DPI screens by default and Qt 5 needs `AA_EnableHighDpiScaling`
(the preview enables it); in both cases the scale factor can be non-integer
(e.g. 1.25, 1.5), which Qt 6 uses as-is by default (`PassThrough`).

The styles scale their fixed pixel metrics with `QStyleHelper::dpiScaled()`
(a `value * dpi / 96` helper shared by all plugins in `shared/qstylehelper_p.h`),
so scroll bars, indicators, sliders and paddings grow with the font DPI just like
Qt's own styles. `oldschool` already delegated most of its metrics to
`QCommonStyle` (which is DPI-aware in both Qt 5.15 and Qt 6); the other styles
now scale their own metrics explicitly.

Two extra things the styles handle for crisp rendering at any scale factor:

* Cached pixmaps (`dirtylooks`, `plastic`, `phase`) are created at the paint
  device's device pixel ratio and tagged with `setDevicePixelRatio()`, and the
  pixmap-cache keys include the dpr so 1x and 2x frames never share a cache
  entry (`shared/qstylecache_p.h`).
* Icons are fetched with `QIcon::pixmap(size, dpr, ...)` on Qt 6 so they resolve
  to their high-resolution versions instead of being scaled up from 1x.

## License

qtstyles is free software, released under the GNU Lesser General Public
License, version 3 (or, at your option, any later version). See the
`LICENSE` file for the full text.

Copyright (C) 2026 Qize Huang <hgoldfish@gmail.com>

The source tree also contains third-party components which keep their
original licenses:

* The `oldschool`, `newschool`, `dirtylooks` and `plastic` styles and the
  shared style helpers are derived from the Qt Toolkit and are subject to
  the Qt LGPL license terms of their original copyright holders (Digia Plc
  and/or The Qt Company Ltd).
* `phase/` is the Phase style by David Johnson, distributed under the MIT
  license.
* `winxp/` is a port of WindowsModernStyle by Michał Męciński, distributed
  under the BSD license (see `winxp/COPYING`).
