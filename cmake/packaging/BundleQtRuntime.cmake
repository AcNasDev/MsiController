if(NOT MSICONTROLLER_BUNDLE_QT_RUNTIME)
    return()
endif()

if(TARGET Qt6::qmake)
    get_target_property(_msicontroller_qmake Qt6::qmake IMPORTED_LOCATION)
endif()

if(NOT _msicontroller_qmake)
    find_program(_msicontroller_qmake NAMES qmake6 qmake REQUIRED)
endif()

function(_msicontroller_qmake_query output_variable query_key)
    execute_process(
        COMMAND "${_msicontroller_qmake}" -query "${query_key}"
        OUTPUT_VARIABLE _query_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )
    set("${output_variable}" "${_query_output}" PARENT_SCOPE)
endfunction()

_msicontroller_qmake_query(MSICONTROLLER_QT_VERSION QT_VERSION)
_msicontroller_qmake_query(MSICONTROLLER_QT_INSTALL_LIBS QT_INSTALL_LIBS)
_msicontroller_qmake_query(MSICONTROLLER_QT_INSTALL_PLUGINS QT_INSTALL_PLUGINS)
_msicontroller_qmake_query(MSICONTROLLER_QT_INSTALL_QML QT_INSTALL_QML)
_msicontroller_qmake_query(MSICONTROLLER_QT_INSTALL_TRANSLATIONS QT_INSTALL_TRANSLATIONS)

message(STATUS "Bundling Qt ${MSICONTROLLER_QT_VERSION} runtime")
message(STATUS "Qt libraries: ${MSICONTROLLER_QT_INSTALL_LIBS}")
message(STATUS "Qt plugins: ${MSICONTROLLER_QT_INSTALL_PLUGINS}")
message(STATUS "Qt QML modules: ${MSICONTROLLER_QT_INSTALL_QML}")

set(MSICONTROLLER_BUNDLED_QT_DIR "qt")

set(_msicontroller_qt_runtime_libs
    Core
    DBus
    Gui
    Network
    OpenGL
    OpenGLWidgets
    Widgets
    Qml
    QmlCore
    QmlMeta
    QmlModels
    QmlWorkerScript
    Quick
    QuickControls2
    QuickControls2Basic
    QuickControls2BasicStyleImpl
    QuickControls2Fusion
    QuickControls2FusionStyleImpl
    QuickControls2Impl
    QuickLayouts
    QuickTemplates2
    QuickEffects
    QuickShapes
    Charts
    ChartsQml
    Svg
    LabsPlatform
    LabsSettings
    XcbQpa
)

set(_msicontroller_qt_lib_patterns)
foreach(_qt_lib IN LISTS _msicontroller_qt_runtime_libs)
    list(APPEND _msicontroller_qt_lib_patterns PATTERN "libQt6${_qt_lib}.so*")
endforeach()
list(APPEND _msicontroller_qt_lib_patterns PATTERN "libicu*.so*")

install(
    DIRECTORY "${MSICONTROLLER_QT_INSTALL_LIBS}/"
    DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/lib"
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
        ${_msicontroller_qt_lib_patterns}
        PATTERN "cmake" EXCLUDE
        PATTERN "pkgconfig" EXCLUDE
)

function(_msicontroller_install_qt_plugin relative_plugin_path)
    set(_plugin_path "${MSICONTROLLER_QT_INSTALL_PLUGINS}/${relative_plugin_path}")
    if(EXISTS "${_plugin_path}")
        get_filename_component(_plugin_dir "${relative_plugin_path}" DIRECTORY)
        install(
            FILES "${_plugin_path}"
            DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/plugins/${_plugin_dir}"
        )
    endif()
endfunction()

_msicontroller_install_qt_plugin("iconengines/libqsvgicon.so")
_msicontroller_install_qt_plugin("imageformats/libqgif.so")
_msicontroller_install_qt_plugin("imageformats/libqico.so")
_msicontroller_install_qt_plugin("imageformats/libqjpeg.so")
_msicontroller_install_qt_plugin("imageformats/libqsvg.so")
_msicontroller_install_qt_plugin("platforms/libqminimal.so")
_msicontroller_install_qt_plugin("platforms/libqoffscreen.so")
_msicontroller_install_qt_plugin("platforms/libqxcb.so")
_msicontroller_install_qt_plugin("platformthemes/libqxdgdesktopportal.so")
_msicontroller_install_qt_plugin("xcbglintegrations/libqxcb-egl-integration.so")
_msicontroller_install_qt_plugin("xcbglintegrations/libqxcb-glx-integration.so")

function(_msicontroller_install_qml_module relative_qml_path)
    set(_qml_path "${MSICONTROLLER_QT_INSTALL_QML}/${relative_qml_path}")
    if(EXISTS "${_qml_path}")
        install(
            DIRECTORY "${_qml_path}/"
            DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/qml/${relative_qml_path}"
            USE_SOURCE_PERMISSIONS
        )
    endif()
endfunction()

install(
    DIRECTORY "${MSICONTROLLER_QT_INSTALL_QML}/QtQml/"
    DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/qml/QtQml"
    USE_SOURCE_PERMISSIONS
    PATTERN "XmlListModel" EXCLUDE
)

install(
    DIRECTORY "${MSICONTROLLER_QT_INSTALL_QML}/QtQuick/"
    DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/qml/QtQuick"
    USE_SOURCE_PERMISSIONS
    PATTERN "Controls/designer" EXCLUDE
    PATTERN "Controls/FluentWinUI3" EXCLUDE
    PATTERN "Controls/Imagine" EXCLUDE
    PATTERN "Controls/Material" EXCLUDE
    PATTERN "Controls/Universal" EXCLUDE
    PATTERN "Dialogs" EXCLUDE
    PATTERN "LocalStorage" EXCLUDE
    PATTERN "Particles" EXCLUDE
    PATTERN "Shapes/DesignHelpers" EXCLUDE
    PATTERN "VectorImage" EXCLUDE
    PATTERN "tooling" EXCLUDE
)

_msicontroller_install_qml_module("Qt/labs/platform")
_msicontroller_install_qml_module("Qt/labs/settings")
_msicontroller_install_qml_module("QtCharts")
_msicontroller_install_qml_module("QtCore")

foreach(_qml_metadata IN ITEMS builtins.qmltypes jsroot.qmltypes)
    if(EXISTS "${MSICONTROLLER_QT_INSTALL_QML}/${_qml_metadata}")
        install(
            FILES "${MSICONTROLLER_QT_INSTALL_QML}/${_qml_metadata}"
            DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/qml"
        )
    endif()
endforeach()

if(EXISTS "${MSICONTROLLER_QT_INSTALL_TRANSLATIONS}")
    install(
        DIRECTORY "${MSICONTROLLER_QT_INSTALL_TRANSLATIONS}/"
        DESTINATION "${MSICONTROLLER_BUNDLED_QT_DIR}/translations"
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
            PATTERN "qt_*.qm"
            PATTERN "qtbase_*.qm"
            PATTERN "qtdeclarative_*.qm"
    )
endif()
