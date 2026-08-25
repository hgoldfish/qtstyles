# preview — qtstyles widget style previewer

A small Qt application that shows the styles shipped with qtstyles
(`dirtylooks`, `oldschool`, `newschool`, `highschool`, `plastic`, `phase`,
`winxp` and its blue/silver/olive variants) applied to a window packed with as
many Qt widgets as practical: buttons, inputs, lists, trees, tables, progress
bars, sliders, tabs, toolboxes, text editors, frames, calendars, standard
dialogs, menus, toolbars and docks.

The styles are compiled in directly, so the previewer works without first
installing the style plugins.

## Build

The app supports both Qt 5 (qmake) and Qt 6 (CMake or qmake).

### Qt 6 (CMake)

From the repository root:

```sh
cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt6 --target preview -j$(nproc)
./build-qt6/preview/preview
```

> Note: `plasticstyle.cpp` reads a platform theme hint through Qt private
> headers, so it links `Qt6::GuiPrivate`; the matching private-headers
> package must be installed (same requirement as the `plastic` plugin).

### Qt 5 (qmake)

```sh
cd preview
qmake
make
./preview
```

### Qt 6 via qmake

```sh
cd preview
qmake6
make
./preview
```

## Usage

Switch styles at any time from the **Style:** drop-down in the toolbar, or
start with a specific style:

```sh
./preview --style phase
```

Each built-in style carries its own classic colour palette
(`QStyle::standardPalette()`), but it is a *suggestion* only: by default the
previewer keeps the system palette and just swaps the drawing style. To see a
style's classic palette, tick **Tools → Apply Classic Palette** — the current
style's `standardPalette()` is applied right away, and every later style switch
re-applies the new style's palette as long as the option stays checked.
Unchecking restores the startup (system) palette.

Every `standardPalette()` is self-contained (all roles are set explicitly), so
switching styles never leaks colours from the previously applied palette.

The three `winxp-blue`, `winxp-silver` and `winxp-olive` variants are the
exception: their Luna palette is hard-coded and applied when the style is
installed, so they always show their fixed colours no matter how the option is
set.

The `Misc` tab shows the standard Qt dialogs (`QMessageBox`, `QColorDialog`,
`QFontDialog`, `QFileDialog`, `QInputDialog`, `QProgressDialog`) rendered in
the active style, so the whole look can be evaluated.
