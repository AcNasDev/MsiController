cmake_minimum_required(VERSION 3.16)

project(helper VERSION "${MSICONTROLLER_VERSION}" LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus)

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ecparametersabstract.h
                           ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Parameters.xml
                           OPTIONS -m -s -p)
qt_add_dbus_adaptor(ecservice_SRCS
                   ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Parameters.xml
                   ecparametersabstract.h
                   EcParametersAbstract)
qt_add_dbus_interface(ecservice_SRCS
                    ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Parameters.xml
                    ecparametersinterface)

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ecmemoryabstract.h
                           ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Memory.xml
                           OPTIONS -m -s -p)
qt_add_dbus_adaptor(ecservice_SRCS
                   ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Memory.xml
                   ecmemoryabstract.h
                   EcMemoryAbstract)
qt_add_dbus_interface(ecservice_SRCS
                    ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Memory.xml
                    ecmemoryinterface)

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ecprofilesabstract.h
                           ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Profiles.xml
                           OPTIONS -m -s -p)
qt_add_dbus_adaptor(ecservice_SRCS
                   ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Profiles.xml
                   ecprofilesabstract.h
                   EcProfilesAbstract)
qt_add_dbus_interface(ecservice_SRCS
                    ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Profiles.xml
                    ecprofilesinterface)

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/echealthabstract.h
                           ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Health.xml
                           OPTIONS -m -s -p)
qt_add_dbus_adaptor(ecservice_SRCS
                   ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Health.xml
                   echealthabstract.h
                   EcHealthAbstract)
qt_add_dbus_interface(ecservice_SRCS
                    ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.Health.xml
                    echealthinterface)

add_library(helper SHARED
    struct.h
    dbusapi.h
    dbuscodec.h
    logging.h
    logging.cpp
    ecparametersabstract.h
    ecmemoryabstract.h
    ecprofilesabstract.h
    echealthabstract.h
    ${ecservice_SRCS}
)

target_include_directories(helper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                       ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(helper PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

target_compile_definitions(helper PRIVATE HELPER_LIBRARY)

include(GNUInstallDirs)
install(TARGETS helper
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(NOT TARGET uninstall_helper)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall_helper
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()
