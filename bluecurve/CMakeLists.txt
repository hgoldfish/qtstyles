qt_add_plugin(bluecurvestyle
    bluecurvestyle.cpp
    plugin.cpp
)
target_link_libraries(bluecurvestyle PRIVATE Qt6::Widgets)
target_include_directories(bluecurvestyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

install(TARGETS bluecurvestyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
