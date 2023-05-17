TARGET  = oldschoolstyle
PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = OldschoolPlugin
load(qt_plugin)

QT = core gui widgets

HEADERS += newschoolstyle.h
HEADERS += oldschoolstyle.h
SOURCES += newschoolstyle.cpp
SOURCES += oldschoolstyle.cpp
SOURCES += plugin.cpp

OTHER_FILES += oldschool.json
