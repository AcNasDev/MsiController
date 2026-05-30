cmake_minimum_required(VERSION 3.16)

set(MSICONTROLLER_FALLBACK_VERSION "0.1.0" CACHE STRING "Version used when no Git tag is available")
include(cmaketools/CMakeToolsVersionFromGit.cmake)

if(NOT CMAKE_TOOLS_GIT_FOUND_TAG)
    string(REPLACE "." ";" _msicontroller_fallback_version "${MSICONTROLLER_FALLBACK_VERSION}")
    list(GET _msicontroller_fallback_version 0 CMAKE_TOOLS_GIT_TAG_MAJOR)
    list(GET _msicontroller_fallback_version 1 CMAKE_TOOLS_GIT_TAG_MINOR)
    list(GET _msicontroller_fallback_version 2 CMAKE_TOOLS_GIT_TAG_PATCH)
    set(CMAKE_TOOLS_GIT_DISTANCE 0)
endif()

if(CMAKE_TOOLS_GIT_TAG_MINOR STREQUAL "")
    set(CMAKE_TOOLS_GIT_TAG_MINOR 0)
endif()
if(CMAKE_TOOLS_GIT_TAG_PATCH STREQUAL "")
    set(CMAKE_TOOLS_GIT_TAG_PATCH 0)
endif()

set(MSICONTROLLER_VERSION "${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR}.${CMAKE_TOOLS_GIT_TAG_PATCH}")

project(MsiController VERSION "${MSICONTROLLER_VERSION}" LANGUAGES CXX)
include(GNUInstallDirs)
add_definitions(-DCMAKE_TOOLS_GIT_TAG_MAJOR="${CMAKE_TOOLS_GIT_TAG_MAJOR}")
add_definitions(-DCMAKE_TOOLS_GIT_TAG_MINOR="${CMAKE_TOOLS_GIT_TAG_MINOR}")
add_definitions(-DCMAKE_TOOLS_GIT_TAG_PATCH="${CMAKE_TOOLS_GIT_TAG_PATCH}")

option(MSICONTROLLER_BUNDLE_QT_RUNTIME "Bundle Qt runtime libraries, plugins and QML modules into the package" OFF)

set(MSICONTROLLER_PACKAGE_NAME "msicontroller")
set(MSICONTROLLER_DKMS_MODULE_NAME "msiecmodule")
set(MSICONTROLLER_DKMS_VERSION "${PROJECT_VERSION}")
set(MSICONTROLLER_CLIENT_BINARY "${CMAKE_INSTALL_FULL_BINDIR}/MsiControlCenterClient")
set(MSICONTROLLER_SERVICE_BINARY "${CMAKE_INSTALL_FULL_BINDIR}/MsiControlCenterService")

if(MSICONTROLLER_BUNDLE_QT_RUNTIME)
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib;\$ORIGIN/../qt/lib")
    set(MSICONTROLLER_CLIENT_EXEC "${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-client")
    set(MSICONTROLLER_SERVICE_EXEC "${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-service")
else()
    set(MSICONTROLLER_CLIENT_EXEC "${MSICONTROLLER_CLIENT_BINARY}")
    set(MSICONTROLLER_SERVICE_EXEC "${MSICONTROLLER_SERVICE_BINARY}")
endif()

add_subdirectory(src/helper)
add_subdirectory(src/module)
add_subdirectory(src/service)
add_subdirectory(src/client)

if(MSICONTROLLER_BUNDLE_QT_RUNTIME)
    install(CODE "
        file(REMOVE \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-client\")
        file(REMOVE \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-service\")
        file(CREATE_LINK \"MsiControlCenterClient\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-client\" SYMBOLIC)
        file(CREATE_LINK \"MsiControlCenterService\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-service\" SYMBOLIC)
    ")
endif()

add_custom_target(uninstall
    DEPENDS uninstall_module uninstall_client uninstall_service uninstall_helper
)

include(cmake/packaging/PackageConfig.cmake)
