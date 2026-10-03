/****************************************************************************
**
** Copyright (C) 2026 Qize Huang <hgoldfish@gmail.com>
**
** This file is part of qtstyles, a collection of retro Qt widget styles.
**
** This library is free software: you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library.  If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

// qtstyles preview — entry point
#include <QApplication>
#include <QByteArray>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QStandardPaths>
#include <QStyle>
#include <QStringList>
#include <QTimer>

#include "previewwindow.h"

// QApplication consumes a "-style <name>" argument itself (it looks up a
// style plugin) and removes it from argv, so our own "--style" would never
// reach QCommandLineParser. Remap it to "--widget-style" before the
// QApplication is constructed.
static void remapStyleArguments(int argc, char *argv[], QList<QByteArray> &replacements)
{
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg == "-style" || arg == "--style") {
            argv[i] = const_cast<char *>("--widget-style");
        } else if (arg.startsWith("--style=") || arg.startsWith("-style=")) {
            const int eq = arg.indexOf('=');
            replacements.append(QByteArray("--widget-style=" + arg.mid(eq + 1)));
            argv[i] = replacements.last().data();
        }
    }
}

// The offscreen / minimal / vnc platform plugins expose no icon theme at all:
// QIcon::themeName() is empty and QIcon::themeSearchPaths() is just ":/icons".
// In that case every QIcon::fromTheme() returns a null icon, and the toolbar
// and menus fall back to showing their text labels instead of icons. When that
// happens, point Qt at the standard freedesktop icon directories and pick an
// installed theme so the headless screenshots (docs/screenshots.py) still show
// icons. On a real desktop (xcb/wayland) the platform already supplies a theme,
// so this is a no-op.
static QStringList freedesktopIconSearchPaths()
{
    QStringList paths;
    // QStandardPaths already knows XDG_DATA_HOME, XDG_DATA_DIRS, the distro
    // defaults and the flatpak export dirs, so don't re-parse them by hand.
    const QStringList dataLocations =
        QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &dir : dataLocations)
        paths << dir + QStringLiteral("/icons");
    // Legacy per-user location, still consulted by some icon loaders.
    paths << QDir::homePath() + QStringLiteral("/.icons");
    return paths;
}

static QString pickInstalledIconTheme(const QStringList &searchPaths)
{
    // Names shared by the common freedesktop themes, tried in order. Qt's own
    // fallback (empty under offscreen) wins if it is set.
    QStringList candidates;
    const QString fallback = QIcon::fallbackThemeName();
    if (!fallback.isEmpty())
        candidates << fallback;
    candidates << QStringLiteral("breeze") << QStringLiteral("breeze-dark")
               << QStringLiteral("oxygen") << QStringLiteral("Adwaita")
               << QStringLiteral("hicolor");

    for (const QString &name : candidates) {
        for (const QString &base : searchPaths) {
            if (QFileInfo::exists(base + QLatin1Char('/') + name
                                  + QStringLiteral("/index.theme")))
                return name;
        }
    }

    // Last resort: any theme directory that carries an index.theme.
    for (const QString &base : searchPaths) {
        QDirIterator it(base, QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            const QString dir = it.next();
            if (QFileInfo::exists(dir + QStringLiteral("/index.theme")))
                return QFileInfo(dir).fileName();
        }
    }
    return QString();
}

static void ensureIconTheme()
{
    if (!QIcon::themeName().isEmpty())
        return;

    QStringList searchPaths = QIcon::themeSearchPaths();
    for (const QString &dir : freedesktopIconSearchPaths()) {
        if (QFileInfo(dir).isDir() && !searchPaths.contains(dir))
            searchPaths << dir;
    }
    QIcon::setThemeSearchPaths(searchPaths);

    const QString theme = pickInstalledIconTheme(searchPaths);
    if (!theme.isEmpty()) {
        QIcon::setThemeName(theme);
        QIcon::setFallbackThemeName(QStringLiteral("hicolor"));
    }
}

int main(int argc, char *argv[])
{
    QList<QByteArray> replacements;
    remapStyleArguments(argc, argv, replacements);

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    // Must run before PreviewWindow builds its toolbar/menus, so the
    // QIcon::fromTheme() calls resolve on headless platforms.
    ensureIconTheme();
    QCoreApplication::setApplicationName(QStringLiteral("qtstyles-preview"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("qtstyles"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Preview the qtstyles widget styles "
                                                    "(dirtylooks, oldschool, newschool, highschool, "
                                                    "plastic, phase, winxp, winxp-blue, winxp-silver, "
                                                    "winxp-olive, bluecurve, keramik, platinum; "
                                                    "each also has a -classic variant that forces "
                                                    "its classic palette)."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption styleOption(QStringList() << QStringLiteral("w") << QStringLiteral("widget-style"),
        QStringLiteral("Initial widget style to display (also accepted as --style)."),
        QStringLiteral("name"));
    parser.addOption(styleOption);

    QCommandLineOption tabOption(QStringLiteral("tab"),
        QStringLiteral("Initial preview tab index (0 = Buttons)."),
        QStringLiteral("index"));
    parser.addOption(tabOption);

    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
        QStringLiteral("Save a screenshot of the window to <file> after startup and exit."),
        QStringLiteral("file"));
    parser.addOption(screenshotOption);

    parser.process(app);

    PreviewWindow window(parser.value(styleOption), parser.value(tabOption).toInt());
    window.show();

    const QString screenshotFile = parser.value(screenshotOption);
    if (!screenshotFile.isEmpty()) {
        QTimer::singleShot(1200, &window, [&window, screenshotFile]() {
            qInfo().noquote() << "Style:" << window.style()->metaObject()->className();
            if (window.grab().save(screenshotFile))
                qInfo().noquote() << "Screenshot saved:" << screenshotFile;
            else
                qCritical().noquote() << "Failed to save screenshot:" << screenshotFile;
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
