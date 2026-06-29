cmake_minimum_required(VERSION 3.16)

project(MsiControllerDoctor VERSION "${MSICONTROLLER_VERSION}" LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus)

add_executable(MsiControllerDoctor
    main.cpp
)

target_link_libraries(MsiControllerDoctor PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    helper
)

include(GNUInstallDirs)
install(TARGETS MsiControllerDoctor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(MSICONTROLLER_BUNDLE_QT_RUNTIME)
    install(CODE "
        file(REMOVE \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-doctor\")
        file(CREATE_LINK \"MsiControllerDoctor\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/msicontroller-doctor\" SYMBOLIC)
    ")
endif()

if(NOT TARGET uninstall_doctor)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall_doctor
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
