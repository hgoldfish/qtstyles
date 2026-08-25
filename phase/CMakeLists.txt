qt_add_plugin(phasestyle
    phasestyle.cpp
    plugin.cpp
)
target_link_libraries(phasestyle PRIVATE Qt6::Widgets)

install(TARGETS phasestyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
