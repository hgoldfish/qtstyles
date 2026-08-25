qt_add_plugin(winxpstyle
    winxpstyle.cpp
    plugin.cpp
)
target_link_libraries(winxpstyle PRIVATE Qt6::Widgets)
target_include_directories(winxpstyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

install(TARGETS winxpstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
