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
#include <QStyle>
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

int main(int argc, char *argv[])
{
    QList<QByteArray> replacements;
    remapStyleArguments(argc, argv, replacements);

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qtstyles-preview"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.0"));
    QCoreApplication::setOrganizationName(QStringLiteral("qtstyles"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Preview the qtstyles widget styles "
                                                    "(dirtylooks, oldschool, newschool, highschool, "
                                                    "plastic, phase, winxp, winxp-blue, winxp-silver, "
                                                    "winxp-olive, bluecurve, keramik, platinum)."));
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
