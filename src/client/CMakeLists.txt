cmake_minimum_required(VERSION 3.16)

project(Client VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Charts Core Gui Qml Quick DBus)

qt_standard_project_setup(REQUIRES 6.8)

add_executable(Client
    main.cpp
    esproxy.cpp
    esproxy.h
    proxyparameter.h
    proxyparameter.cpp
    curveutils.cpp
    curveutils.h
    resources.qrc
)

target_include_directories(Client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} 
                                       ${CMAKE_CURRENT_BINARY_DIR})

qt_add_qml_module(Client
    URI client
    VERSION 1.0
    QML_FILES
        Main.qml
        GraphCard.qml
        FanCurveCard.qml
        Sensor.qml
    
)

set_target_properties(Client PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(Client PUBLIC
    Qt6::Quick
    Qt6::DBus
    Qt6::Charts
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    helper
)

include(GNUInstallDirs)
install(TARGETS Client
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
