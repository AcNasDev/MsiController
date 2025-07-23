 function(defaultlib)
    set (options)
    set (oneValueArgs
        LIB_NAME)
    set (multiValueArgs)
    cmake_parse_arguments(VAL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    include(${CMAKE_CURRENT_SOURCE_DIR}/cmaketools/CMakeToolsVersionFromGit.cmake)
if( WIN32 )
    set_target_properties(${VAL_LIB_NAME} PROPERTIES
        OUTPUT_NAME "${VAL_LIB_NAME}"
        VERSION ${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR}.${CMAKE_TOOLS_GIT_TAG_PATCH} )
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmaketools/generate_product_version.cmake)
    generate_product_version(
       VersionFilesOutputVariable
       NAME "${VAL_LIB_NAME}"
       ICON ${CMAKE_CURRENT_SOURCE_DIR}/cmaketools/libs.ico
       VERSION_MAJOR ${CMAKE_TOOLS_GIT_TAG_MAJOR}
       VERSION_MINOR ${CMAKE_TOOLS_GIT_TAG_MINOR}
       VERSION_PATCH ${CMAKE_TOOLS_GIT_TAG_PATCH}
       VERSION_REVISION ${CMAKE_TOOLS_GIT_DISTANCE}
    )
    target_sources(${VAL_LIB_NAME} PRIVATE ${VersionFilesOutputVariable})
else()
    set_target_properties( ${VAL_LIB_NAME} PROPERTIES
        VERSION ${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR}.${CMAKE_TOOLS_GIT_TAG_PATCH}
        SOVERSION ${CMAKE_TOOLS_GIT_TAG_MAJOR} )
endif()

string(TOUPPER ${VAL_LIB_NAME} UPPER_LIB_NAME)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generateexport.cmake"
[=[
file(READ "${SOURCE}" TEXT)
string(REPLACE "NAME" "${LIB_NAME}" TEXT "${TEXT}")
file(WRITE "${TARGET}" "${TEXT}")
]=])
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${VAL_LIB_NAME}_export.h
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE=${CMAKE_CURRENT_SOURCE_DIR}/cmaketools/lib_export.h.in"
        "-DTARGET=${CMAKE_CURRENT_SOURCE_DIR}/${VAL_LIB_NAME}_export.h"
        "-DLIB_NAME=${UPPER_LIB_NAME}"
        -P "${CMAKE_CURRENT_BINARY_DIR}/generateexport.cmake"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmaketools/lib_export.h.in
    VERBATIM
)
target_sources(${VAL_LIB_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${VAL_LIB_NAME}_export.h)
target_compile_definitions(${VAL_LIB_NAME} PRIVATE ${UPPER_LIB_NAME}_LIBRARY)
target_include_directories(${VAL_LIB_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

include_directories(BEFORE SYSTEM ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR})
project(${VAL_LIB_NAME} VERSION ${CMAKE_TOOLS_GIT_TAG_MAJOR}.${CMAKE_TOOLS_GIT_TAG_MINOR} LANGUAGES CXX)
endfunction()
