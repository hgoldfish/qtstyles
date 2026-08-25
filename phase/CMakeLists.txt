qt_add_plugin(phasestyle
    phasestyle.cpp
    plugin.cpp
)
target_link_libraries(phasestyle PRIVATE Qt6::Widgets)
target_include_directories(phasestyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

install(TARGETS phasestyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
