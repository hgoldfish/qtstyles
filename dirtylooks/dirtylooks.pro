TARGET  = dirtylooksstyle
PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = DirtylooksStylePlugin
load(qt_plugin)

QT = core gui widgets

HEADERS += dirtylooksstyle.h
SOURCES += dirtylooksstyle.cpp
SOURCES += plugin.cpp

include(../shared/shared.pri)

OTHER_FILES += dirtylooks.json
