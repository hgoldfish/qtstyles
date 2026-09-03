TARGET = preview
TEMPLATE = app

QT += core core-private gui gui-private widgets

# Keep the project at C++11 so it builds against Qt 5.
CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/previewwindow.cpp \
    ../bluecurve/bluecurvestyle.cpp \
    ../dirtylooks/dirtylooksstyle.cpp \
    ../keramik/keramikstyle.cpp \
    ../oldschool/oldschoolstyle.cpp \
    ../oldschool/newschoolstyle.cpp \
    ../oldschool/highschoolstyle.cpp \
    ../platinum/platinumstyle.cpp \
    ../plastic/plasticstyle.cpp \
    ../phase/phasestyle.cpp \
    ../winxp/winxpstyle.cpp \
    ../shared/qstylehelper.cpp

HEADERS += \
    src/previewwindow.h \
    ../bluecurve/bluecurvestyle.h \
    ../dirtylooks/dirtylooksstyle.h \
    ../keramik/keramikstyle.h \
    ../oldschool/oldschoolstyle.h \
    ../oldschool/newschoolstyle.h \
    ../oldschool/highschoolstyle.h \
    ../platinum/platinumstyle.h \
    ../plastic/plasticstyle.h \
    ../phase/phasestyle.h \
    ../phase/bitmaps.h \
    ../winxp/winxpstyle.h \
    ../shared/qstylehelper_p.h \
    ../shared/qstylecache_p.h \
    ../shared/qhexstring_p.h \
    ../shared/qtstyles_palette.h

INCLUDEPATH += \
    ../bluecurve \
    ../dirtylooks \
    ../keramik \
    ../oldschool \
    ../platinum \
    ../plastic \
    ../phase \
    ../winxp \
    ../shared
