qt_add_plugin(dirtylooksstyle
    dirtylooksstyle.cpp
    plugin.cpp
    ../shared/qstylehelper.cpp
)
target_include_directories(dirtylooksstyle PRIVATE ../lib ../shared)
target_link_libraries(dirtylooksstyle PRIVATE Qt6::Widgets)

install(TARGETS dirtylooksstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
