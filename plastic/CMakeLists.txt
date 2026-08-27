qt_add_plugin(plasticstyle
    plasticstyle.cpp
    plugin.cpp
    ../shared/qstylehelper.cpp
)
target_include_directories(plasticstyle PRIVATE ../lib ../shared)
target_link_libraries(plasticstyle PRIVATE Qt6::Widgets Qt6::GuiPrivate)

install(TARGETS plasticstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
