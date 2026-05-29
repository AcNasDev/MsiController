cmake_minimum_required(VERSION 3.16)

project(MsiController VERSION 0.1 LANGUAGES CXX)
include(GNUInstallDirs)
include(cmaketools/CMakeToolsVersionFromGit.cmake)
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
