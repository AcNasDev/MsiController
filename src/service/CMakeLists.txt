cmake_minimum_required(VERSION 3.16)

project(MsiControlCenterService VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus Concurrent)

qt_standard_project_setup(REQUIRES 6.8)

add_executable(MsiControlCenterService
  main.cpp
  ecservice.cpp
  ecservice.h
  parameter.h
  parameter.cpp
  ioparameter.h
  ioparameter.cpp
  ioparameterimpl.h
  iobuffer.h
  iobuffer.cpp
  cpuparameter.h
  cpuparameter.cpp
  resources.qrc
)

set_target_properties(MsiControlCenterService PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(MsiControlCenterService 
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::DBus
  Qt${QT_VERSION_MAJOR}::Concurrent
  helper
)

include(GNUInstallDirs)
install(TARGETS MsiControlCenterService
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/msi-ec-service.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/msi-ec-service.conf
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-ec-service.conf
    DESTINATION /etc/dbus-1/system.d
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/msi-ec-service.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/msi-ec-service.service
    @ONLY
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msi-ec-service.service
    DESTINATION /lib/systemd/system
)

install(CODE "execute_process(COMMAND systemctl enable msi-ec-service.service)")
install(CODE "execute_process(COMMAND systemctl daemon-reload)")
install(CODE "execute_process(COMMAND systemctl restart msi-ec-service.service)")

if(NOT TARGET uninstall_service)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall_service
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()