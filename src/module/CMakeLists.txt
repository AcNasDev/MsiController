cmake_minimum_required(VERSION 3.10)

set(MODULE_NAME msi_ec_module)
set(MODULE_SRC msiecmodule.c)

execute_process(
    COMMAND uname -r
    OUTPUT_VARIABLE KERNEL_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(KERNEL_SRC /lib/modules/${KERNEL_VERSION}/build)

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