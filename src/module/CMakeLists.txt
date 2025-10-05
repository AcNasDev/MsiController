cmake_minimum_required(VERSION 3.10)

set(MODULE_NAME msi_ec_module)
set(MODULE_SRC msiecmodule.c)

execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_VERSION_UNAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

set(UNAME_KERNEL_PATH "/lib/modules/${KERNEL_VERSION_UNAME}/build")
if(EXISTS ${UNAME_KERNEL_PATH})
    set(KERNEL_VERSION ${KERNEL_VERSION_UNAME})
    message(STATUS "Using kernel version from uname -r: ${KERNEL_VERSION}")
else()
    message(STATUS "Headers for uname -r (${KERNEL_VERSION_UNAME}) not found, searching alternatives...")
    
    file(GLOB KERNEL_BUILD_DIRS "/lib/modules/*/build")
    if(KERNEL_BUILD_DIRS)
        list(GET KERNEL_BUILD_DIRS 0 FIRST_BUILD_DIR)
        get_filename_component(MODULE_DIR ${FIRST_BUILD_DIR} DIRECTORY)
        get_filename_component(KERNEL_VERSION ${MODULE_DIR} NAME)
        message(STATUS "Found alternative kernel headers: ${KERNEL_VERSION}")
    else()
        file(GLOB MODULE_DIRS "/lib/modules/*")
        if(MODULE_DIRS)
            list(GET MODULE_DIRS 0 FIRST_MODULE_DIR)
            get_filename_component(KERNEL_VERSION ${FIRST_MODULE_DIR} NAME)
            message(WARNING "No build directories found, using: ${KERNEL_VERSION} (build may fail)")
        else()
            set(KERNEL_VERSION ${KERNEL_VERSION_UNAME})
            message(WARNING "No kernel modules found, falling back to uname -r: ${KERNEL_VERSION}")
        endif()
    endif()
endif()

set(KERNEL_SRC /lib/modules/${KERNEL_VERSION}/build)

# Финальная проверка
if(NOT EXISTS ${KERNEL_SRC})
    message(FATAL_ERROR "Kernel build directory not found: ${KERNEL_SRC}")
else()
    message(STATUS "Using kernel build directory: ${KERNEL_SRC}")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_SRC}
               ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SRC}
               COPYONLY)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Makefile
"obj-m := msiecmodule.o\n")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SRC}.ko
    COMMAND make -C ${KERNEL_SRC} M=${CMAKE_CURRENT_BINARY_DIR} modules
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SRC}
)

add_custom_target(${MODULE_NAME} ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${MODULE_SRC}.ko
)

add_custom_target(modclean
    COMMAND make -C ${KERNEL_SRC} M=${CMAKE_CURRENT_BINARY_DIR} clean
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/msiecmodule.ko
    DESTINATION /lib/modules/${KERNEL_VERSION}/extra
)

install(CODE "file(WRITE \"\$ENV{DESTDIR}/etc/modules-load.d/msiecmodule.conf\" \"msiecmodule\n\")")

if(NOT TARGET uninstall_module)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY
    )

    add_custom_target(uninstall_module
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )
endif()