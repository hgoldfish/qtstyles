cmake_minimum_required(VERSION 3.16)

project(qtstyles VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Qt6::GuiPrivate is needed by the plastic plugin and the preview app.
# Qt 6.10+ ships private modules as separate CMake packages, so they need
# an explicit find. On Qt <= 6.9 the private target is already exported
# with the public module, so a missing package is not fatal (hence QUIET).
# Using Qt private API is a deliberate choice here (qguiapplication_p.h), so
# silence the "tied to this Qt module build version" configure warning.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 QUIET COMPONENTS GuiPrivate)

qt_standard_project_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(bluecurve)
add_subdirectory(dirtylooks)
add_subdirectory(keramik)
add_subdirectory(oldschool)
add_subdirectory(phase)
add_subdirectory(platinum)
add_subdirectory(plastic)
add_subdirectory(preview)
add_subdirectory(winxp)
