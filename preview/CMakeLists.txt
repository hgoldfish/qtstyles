qt_add_executable(preview
    src/main.cpp
    src/previewwindow.cpp
    src/previewwindow.h
    ../bluecurve/bluecurvestyle.cpp
    ../bluecurve/bluecurvestyle.h
    ../dirtylooks/dirtylooksstyle.cpp
    ../dirtylooks/dirtylooksstyle.h
    ../keramik/keramikstyle.cpp
    ../keramik/keramikstyle.h
    ../oldschool/oldschoolstyle.cpp
    ../oldschool/oldschoolstyle.h
    ../oldschool/newschoolstyle.cpp
    ../oldschool/newschoolstyle.h
    ../oldschool/highschoolstyle.cpp
    ../oldschool/highschoolstyle.h
    ../platinum/platinumstyle.cpp
    ../platinum/platinumstyle.h
    ../plastic/plasticstyle.cpp
    ../plastic/plasticstyle.h
    ../phase/phasestyle.cpp
    ../phase/phasestyle.h
    ../phase/bitmaps.h
    ../winxp/winxpstyle.cpp
    ../winxp/winxpstyle.h
    ../shared/qstylehelper.cpp
    ../shared/qstylehelper_p.h
    ../shared/qstylecache_p.h
    ../shared/qhexstring_p.h
    ../shared/qtstyles_palette.h
)

target_include_directories(preview PRIVATE
    ../bluecurve
    ../dirtylooks
    ../keramik
    ../oldschool
    ../platinum
    ../plastic
    ../phase
    ../winxp
    ../shared
)

# plasticstyle reads a platform theme hint through Qt private headers
target_link_libraries(preview PRIVATE Qt6::Widgets Qt6::GuiPrivate)

install(TARGETS preview RUNTIME DESTINATION bin)
