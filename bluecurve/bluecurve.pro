TARGET  = bluecurvestyle
PLUGIN_TYPE = styles
PLUGIN_CLASS_NAME = BluecurveStylePlugin
load(qt_plugin)

# qt_plugin.prf turns on create_cmake, which would also install a
# Qt5Widgets_*.cmake registration file; we only ship the plugin .so.
CONFIG -= create_cmake

# Qt 5 requires C++11-only code; Qt 6 forces a newer standard on its own.
lessThan(QT_MAJOR_VERSION, 6) {
    CONFIG -= c++14 c++1z c++17 c++2a c++2b
    CONFIG += c++11
}

QT = core gui widgets

# Shared style helpers (qtstyles_palette.h) live in ../lib; the HiDPI
# helpers (qstylehelper_p.h) live in ../shared.
INCLUDEPATH += $$PWD/../lib
INCLUDEPATH += $$PWD/../shared

HEADERS = bluecurvestyle.h
SOURCES = bluecurvestyle.cpp
SOURCES += plugin.cpp

OTHER_FILES += bluecurve.json
