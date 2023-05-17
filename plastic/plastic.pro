TARGET  = plasticstyle
PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = PlasticStylePlugin
load(qt_plugin)

QT = core core-private gui gui-private widgets

HEADERS += plasticstyle.h
SOURCES += plasticstyle.cpp
SOURCES += plugin.cpp

include(../shared/shared.pri)

OTHER_FILES += plastic.json
