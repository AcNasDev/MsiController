cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)

add_executable(parameterregistrytest
    service/parameterregistrytest.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameterregistry.cpp
    ${CMAKE_SOURCE_DIR}/src/service/settingsstore.cpp
)

target_include_directories(parameterregistrytest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/service
)

target_link_libraries(parameterregistrytest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)

add_test(NAME parameterregistrytest COMMAND parameterregistrytest)
