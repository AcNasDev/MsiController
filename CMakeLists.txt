cmake_minimum_required(VERSION 3.16)

project(MsiController VERSION 0.1 LANGUAGES CXX)
include(cmaketools/CMakeToolsVersionFromGit.cmake)
add_definitions(-DCMAKE_TOOLS_GIT_TAG_MAJOR="${CMAKE_TOOLS_GIT_TAG_MAJOR}")
add_definitions(-DCMAKE_TOOLS_GIT_TAG_MINOR="${CMAKE_TOOLS_GIT_TAG_MINOR}")
add_definitions(-DCMAKE_TOOLS_GIT_TAG_PATCH="${CMAKE_TOOLS_GIT_TAG_PATCH}")

add_subdirectory(src/helper)
add_subdirectory(src/module)
add_subdirectory(src/service)
add_subdirectory(src/client)

add_custom_target(uninstall
    DEPENDS uninstall_module uninstall_client uninstall_service uninstall_helper uninstall_module
)