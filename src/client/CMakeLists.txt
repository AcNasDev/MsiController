cmake_minimum_required(VERSION 3.16)

project(MsiControlCenterClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Charts Core Gui Qml Quick DBus)

qt_standard_project_setup(REQUIRES 6.8)

add_executable(MsiControlCenterClient
    main.cpp
    esproxy.cpp
    esproxy.h
    proxyparameter.h
    proxyparameter.cpp
    curveutils.cpp
    curveutils.h
    resources.qrc
)

qt_add_qml_module(MsiControlCenterClient
    URI client
    VERSION 1.0
    QML_FILES
        Main.qml
        GraphCard.qml
        FanCurveCard.qml
        Sensor.qml
    
)

set_target_properties(MsiControlCenterClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(MsiControlCenterClient PUBLIC
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    helper
)

include(GNUInstallDirs)
install(TARGETS MsiControlCenterClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/msi-control-center.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center.desktop
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/icon/logo.svg
    ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center.svg
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
)

if(NOT TARGET uninstall_client)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall_client
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
