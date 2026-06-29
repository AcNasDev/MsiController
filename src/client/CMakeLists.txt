cmake_minimum_required(VERSION 3.16)

project(MsiControlCenterClient VERSION "${MSICONTROLLER_VERSION}" LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Qml Quick DBus Widgets)
set(MSICONTROLLER_QT_DIR_VAR "Qt${QT_VERSION_MAJOR}_DIR")
get_filename_component(MSICONTROLLER_QT_PREFIX "${${MSICONTROLLER_QT_DIR_VAR}}/../../.." ABSOLUTE)
find_program(MSICONTROLLER_QT_LRELEASE
    NAMES lrelease
    PATHS
        /usr/lib/qt6/bin
        "${MSICONTROLLER_QT_PREFIX}/bin"
    NO_DEFAULT_PATH
)
if(MSICONTROLLER_QT_LRELEASE)
    set(QT_LRELEASE_EXECUTABLE "${MSICONTROLLER_QT_LRELEASE}" CACHE FILEPATH "Qt lrelease executable" FORCE)
else()
    find_program(QT_LRELEASE_EXECUTABLE lrelease)
endif()

qt_standard_project_setup(REQUIRES 6.8)

add_executable(MsiControlCenterClient
    main.cpp
    esproxy.cpp
    esproxy.h
    autoprofilecontroller.h
    autoprofilecontroller.cpp
    behaviorprofilecontroller.h
    behaviorprofilecontroller.cpp
    telemetryhistorycontroller.h
    telemetryhistorycontroller.cpp
    appsettingscontroller.h
    appsettingscontroller.cpp
    parameterclient.h
    parameterclient.cpp
    deviceprofileclient.h
    deviceprofileclient.cpp
    ecmemoryclient.h
    ecmemoryclient.cpp
    diagnosticsclient.h
    diagnosticsclient.cpp
    proxyparameter.h
    proxyparameter.cpp
    curveutils.cpp
    curveutils.h
    gpucpuperformancegraph.cpp
    gpucpuperformancegraph.h
    gpulinechart.cpp
    gpulinechart.h
    resources.qrc
)

qt_add_qml_module(MsiControlCenterClient
    URI client
    VERSION 1.0
    QML_FILES
        Main.qml
        AppCard.qml
        MetricTile.qml
        ToggleTile.qml
        OptionGroup.qml
        CurrentBehaviorCard.qml
        TargetTemperatureCard.qml
        GpuControlsCard.qml
        DiagnosticsPage.qml
        DeviceProfilesPage.qml
        MemoryDebugPage.qml
        CpuPerformanceGraph.qml
        CpuPage.qml
        GraphCard.qml
        FanCurveCard.qml
        Sensor.qml
    
)

if(QT_LRELEASE_EXECUTABLE)
    set(MSICONTROLLER_RU_TS "${CMAKE_CURRENT_SOURCE_DIR}/translations/msicontroller_ru_RU.ts")
    set(MSICONTROLLER_RU_QM "${CMAKE_CURRENT_BINARY_DIR}/msicontroller_ru_RU.qm")
    add_custom_command(
        OUTPUT "${MSICONTROLLER_RU_QM}"
        COMMAND "${QT_LRELEASE_EXECUTABLE}" "${MSICONTROLLER_RU_TS}" -qm "${MSICONTROLLER_RU_QM}"
        DEPENDS "${MSICONTROLLER_RU_TS}"
        VERBATIM
    )
    set_source_files_properties("${MSICONTROLLER_RU_QM}" PROPERTIES
        QT_RESOURCE_ALIAS "msicontroller_ru_RU.qm"
    )
    qt_add_resources(MsiControlCenterClient "translations"
        PREFIX "/i18n"
        FILES "${MSICONTROLLER_RU_QM}"
    )
endif()

set_target_properties(MsiControlCenterClient PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(MsiControlCenterClient PUBLIC
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Widgets
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
    DESTINATION /usr/share/applications
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center.svg
    DESTINATION /usr/share/icons/hicolor/scalable/apps
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/msi-control-center-autostart.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center-autostart.desktop
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-control-center-autostart.desktop
    DESTINATION /etc/xdg/autostart
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
