cmake_minimum_required(VERSION 3.16)

project(helper LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus)

qt_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/ecserviceabstract.h
                           ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.xml
                           OPTIONS -m -s -p)
qt_add_dbus_adaptor(ecservice_SRCS
                   ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.xml
                   ecserviceabstract.h
                   EcServiceAbstract)
qt_add_dbus_interface(ecservice_SRCS
                    ${CMAKE_CURRENT_BINARY_DIR}/com.msi.ec.xml
                    ecinterface)

add_library(helper SHARED
    struct.h
    ecserviceabstract.h
    ${ecservice_SRCS}
)

target_include_directories(helper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
                                       ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(helper PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
)

target_compile_definitions(helper PRIVATE HELPER_LIBRARY)
