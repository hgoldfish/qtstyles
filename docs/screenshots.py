#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate the theme screenshots used by docs/themes.md.

For every *-classic style shipped with qtstyles this script runs the
preview program (offscreen, no display needed) on every preview tab and
saves a PNG under docs/screenshots/<style>/<tab>.png.

Run from the repository root:

    python3 docs/screenshots.py            # reuse the existing preview binary
    python3 docs/screenshots.py --build    # rebuild preview first (after editing the styles)
    python3 docs/screenshots.py --build-dir build-qt5/preview

The preview binary is built with:

    cmake -S . -B build-qt6 -DCMAKE_BUILD_TYPE=Release
    cmake --build build-qt6 --target preview
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

# Only the -classic variants, which force their classic palette at install
# time, so the screenshots always show the look described in docs/themes.md
# regardless of the host theme.
STYLES = [
    "bluecurve-classic",
    "dirtylooks-classic",
    "highschool-classic",
    "keramik-classic",
    "newschool-classic",
    "oldschool-classic",
    "phase-classic",
    "plastic-classic",
    "platinum-classic",
    "winxp-classic",
]

# (preview tab index, file stem) — mirror of the central QTabWidget pages in
# preview/src/previewwindow.cpp::buildCentral().
TABS = [
    (0, "buttons"),
    (1, "inputs"),
    (2, "lists-trees"),
    (3, "progress-sliders"),
    (4, "tabs-panels"),
    (5, "text"),
    (6, "frames-groups"),
    (7, "misc"),
]

OUT_DIR = REPO_ROOT / "docs" / "screenshots"


def default_preview_binary():
    for rel in ("build-qt6/preview/preview", "build-qt5/preview/preview"):
        candidate = REPO_ROOT / rel
        if candidate.is_file():
            return candidate
    return None


def build_preview(build_dir):
    """Configure and build the preview target in build_dir (a CMake build tree)."""
    build_dir = Path(build_dir)
    if not (build_dir / "CMakeCache.txt").exists():
        subprocess.run(
            ["cmake", "-S", str(REPO_ROOT), "-B", str(build_dir), "-DCMAKE_BUILD_TYPE=Release"],
            check=True,
        )
    subprocess.run(
        ["cmake", "--build", str(build_dir), "--target", "preview", "-j", str(os.cpu_count() or 4)],
        check=True,
    )
    return build_dir / "preview" / "preview"


def capture(preview, style, tab_index, out_path):
    """Run the preview once and save a screenshot of one style/tab page."""
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
    proc = subprocess.run(
        [
            str(preview),
            "--style", style,
            "--tab", str(tab_index),
            "--screenshot", str(out_path),
        ],
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            "preview failed for {!r} tab {} (exit {}):\n{}".format(
                style, tab_index, proc.returncode, proc.stderr.strip()
            )
        )
    if not out_path.is_file() or out_path.stat().st_size == 0:
        raise RuntimeError("no screenshot produced for {!r} tab {}".format(style, tab_index))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        help="CMake build tree containing the preview target "
             "(default: build-qt6, falling back to build-qt5)",
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="rebuild the preview target first (use after editing the style "
             "sources so the screenshots reflect the new code)",
    )
    parser.add_argument(
        "--style",
        action="append",
        choices=STYLES,
        help="only capture these styles (repeatable; default: all)",
    )
    parser.add_argument(
        "--tab",
        action="append",
        choices=[name for _, name in TABS],
        help="only capture these tabs (repeatable; default: all)",
    )
    args = parser.parse_args()

    if args.build_dir:
        preview = Path(args.build_dir) / "preview" / "preview"
        if not preview.is_file():
            if not args.build:
                parser.error("{} does not exist; use --build to build it".format(preview))
            preview = build_preview(Path(args.build_dir))
    else:
        preview = default_preview_binary()
        if preview is None:
            if not args.build:
                parser.error(
                    "no preview binary found (build-qt6/preview/preview or "
                    "build-qt5/preview/preview); use --build to build it"
                )
            preview = build_preview(REPO_ROOT / "build-qt6")
        elif args.build:
            # preview resolves to <build-tree>/preview/preview, so the build
            # tree root is two levels up.
            build_root = preview.parents[1]
            subprocess.run(
                ["cmake", "--build", str(build_root), "--target", "preview", "-j", str(os.cpu_count() or 4)],
                check=True,
            )

    styles = args.style or STYLES
    tabs = [(i, name) for i, name in TABS if name in (args.tab or [name for _, name in TABS])]

    total = len(styles) * len(tabs)
    done = 0
    for style in styles:
        for tab_index, tab_name in tabs:
            out_path = OUT_DIR / style / "{}.png".format(tab_name)
            out_path.parent.mkdir(parents=True, exist_ok=True)
            capture(preview, style, tab_index, out_path)
            done += 1
            print("[{}/{}] {} -> {}".format(done, total, style, out_path.relative_to(REPO_ROOT)))

    print("All screenshots saved under {}".format(OUT_DIR.relative_to(REPO_ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
