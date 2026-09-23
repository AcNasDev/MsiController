cmake_minimum_required(VERSION 3.16)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test DBus Qml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test DBus Qml)

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

add_executable(systemaccesstest
    service/systemaccesstest.cpp
    ${CMAKE_SOURCE_DIR}/src/service/systemaccess.cpp
)
target_include_directories(systemaccesstest PRIVATE ${CMAKE_SOURCE_DIR}/src/service)
target_link_libraries(systemaccesstest PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test)
add_test(NAME systemaccesstest COMMAND systemaccesstest)

add_executable(dbuscontracttest
    service/dbuscontracttest.cpp
    ${CMAKE_SOURCE_DIR}/src/service/dbusendpoints.cpp
    ${CMAKE_SOURCE_DIR}/src/service/diagnosticsservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameterregistry.cpp
    ${CMAKE_SOURCE_DIR}/src/service/settingsstore.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecmemoryservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecmemorybackend.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecwritepolicy.cpp
    ${CMAKE_SOURCE_DIR}/src/service/iobuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/service/deviceprofileservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/supportconfig.cpp
)

target_include_directories(dbuscontracttest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/service
)

target_link_libraries(dbuscontracttest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Test
    helper
)

find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
if(NOT DBUS_RUN_SESSION_EXECUTABLE)
    message(FATAL_ERROR "dbus-run-session is required to run dbuscontracttest")
endif()
add_test(NAME dbuscontracttest
    COMMAND "${DBUS_RUN_SESSION_EXECUTABLE}" -- $<TARGET_FILE:dbuscontracttest>
)

add_executable(fakeecbackendtest
    service/fakeecbackendtest.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecsysregistrar.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecparameterfactory.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecmemoryservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecmemorybackend.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ecwritepolicy.cpp
    ${CMAKE_SOURCE_DIR}/src/service/iobuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/service/ioparameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/parameterregistry.cpp
    ${CMAKE_SOURCE_DIR}/src/service/settingsstore.cpp
    ${CMAKE_SOURCE_DIR}/src/service/supportconfig.cpp
    ${CMAKE_SOURCE_DIR}/src/service/deviceprofileservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/deviceprofileapplier.cpp
    ${CMAKE_SOURCE_DIR}/src/service/diagnosticsservice.cpp
    ${CMAKE_SOURCE_DIR}/src/service/fantargetcontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/service/softwareparameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/cpuparameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/cpucontrolparameter.cpp
    ${CMAKE_SOURCE_DIR}/src/service/cpufiles.cpp
    ${CMAKE_SOURCE_DIR}/src/service/systemaccess.cpp
)

target_include_directories(fakeecbackendtest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/service
)

target_compile_definitions(fakeecbackendtest PRIVATE
    SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(fakeecbackendtest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    helper
)

add_test(NAME fakeecbackendtest COMMAND fakeecbackendtest)

add_executable(qmlsmoketest
    client/qmlsmoketest.cpp
    ${CMAKE_SOURCE_DIR}/src/client/esproxy.cpp
    ${CMAKE_SOURCE_DIR}/src/client/parameterclient.cpp
    ${CMAKE_SOURCE_DIR}/src/client/deviceprofileclient.cpp
    ${CMAKE_SOURCE_DIR}/src/client/ecmemoryclient.cpp
    ${CMAKE_SOURCE_DIR}/src/client/diagnosticsclient.cpp
    ${CMAKE_SOURCE_DIR}/src/client/autoprofilecontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/client/behaviorprofilecontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/client/telemetryhistorycontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/client/appsettingscontroller.cpp
    ${CMAKE_SOURCE_DIR}/src/client/proxyparameter.cpp
)

target_include_directories(qmlsmoketest PRIVATE
    ${CMAKE_SOURCE_DIR}/src/client
)

target_link_libraries(qmlsmoketest PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Test
    helper
)

add_test(NAME qmlsmoketest COMMAND qmlsmoketest)

add_test(NAME clientruntime_smoke
    COMMAND ${CMAKE_COMMAND} -E env
        QT_QPA_PLATFORM=offscreen
        QT_FORCE_STDERR_LOGGING=1
        $<TARGET_FILE:MsiControlCenterClient>
        --smoke-test
)
