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

// Qt 5/6 plugin interface for the Bluecurve style. The look is based on the
// Bluecurve theme by Red Hat, Inc. (2002); clean-room implementation written
// for qtstyles, no code from the original or from community ports is included.

#include <QStylePlugin>
#include "bluecurvestyle.h"

class BluecurveStylePlugin : public QStylePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QStyleFactoryInterface" FILE "bluecurve.json")
public:
    QStyle *create(const QString &key) override;
};

QStyle *BluecurveStylePlugin::create(const QString &key)
{
    if (key.compare(QStringLiteral("bluecurve"), Qt::CaseInsensitive) == 0)
        return new BluecurveStyle;
    if (key.compare(QStringLiteral("bluecurve-classic"), Qt::CaseInsensitive) == 0)
        return new BluecurveStyle(true);
    return nullptr;
}

#include "plugin.moc"
