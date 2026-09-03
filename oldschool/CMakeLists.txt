qt_add_plugin(oldschoolstyle
    oldschoolstyle.cpp
    newschoolstyle.cpp
    highschoolstyle.cpp
    plugin.cpp
)
target_link_libraries(oldschoolstyle PRIVATE Qt6::Widgets)
target_include_directories(oldschoolstyle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)

install(TARGETS oldschoolstyle DESTINATION "${QT6_INSTALL_PLUGINS}/styles")
