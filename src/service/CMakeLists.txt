cmake_minimum_required(VERSION 3.16)

project(Service LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core DBus Concurrent)

add_executable(Service
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
  resources.qrc
)

target_link_libraries(Service 
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::DBus
  Qt${QT_VERSION_MAJOR}::Concurrent
  helper
)

include(GNUInstallDirs)
install(TARGETS Service
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
