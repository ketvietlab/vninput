cmake_minimum_required(VERSION 3.24)
if(CMAKE_HOST_APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version" FORCE)
endif()
project(VNInput VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(VNINPUT_BUILD_SETTINGS "Build the Qt Widgets settings application" ON)
option(VNINPUT_DEPLOY_QT "Bundle Qt dependencies during installation" OFF)
option(VNINPUT_BUILD_MACOS_IME "Build the native macOS InputMethodKit adapter" ${APPLE})
if(APPLE AND VNINPUT_BUILD_MACOS_IME)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()
include(CTest)
include(GNUInstallDirs)

add_library(vninput_warnings INTERFACE)
if(MSVC)
    target_compile_options(vninput_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(vninput_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(core)
add_subdirectory(platforms)

if(VNINPUT_BUILD_SETTINGS)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
    qt_standard_project_setup(REQUIRES 6.5)
    add_subdirectory(apps/settings)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

install(FILES THIRD_PARTY_NOTICES.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vninput)
install(FILES third_party/licenses/ket-design-system.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/vninput/licenses)
