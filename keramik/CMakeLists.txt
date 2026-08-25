qt_add_plugin(keramikstyle
    keramikstyle.cpp
    plugin.cpp
)
target_link_libraries(keramikstyle PRIVATE Qt6::Widgets)
target_include_directories(keramikstyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

install(TARGETS keramikstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
