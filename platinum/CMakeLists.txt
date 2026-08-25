qt_add_plugin(platinumstyle
    platinumstyle.cpp
    plugin.cpp
)
target_link_libraries(platinumstyle PRIVATE Qt6::Widgets)
target_include_directories(platinumstyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

install(TARGETS platinumstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
