set(CPACK_PACKAGE_NAME "${MSICONTROLLER_PACKAGE_NAME}")
set(CPACK_PACKAGE_VENDOR "AcNas")
set(CPACK_PACKAGE_CONTACT "mikhail@acnas.net")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MSI laptop control center with a DKMS-managed EC kernel module")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/AcNasDev/MsiController")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(MSICONTROLLER_PACKAGE_RELEASE "1" CACHE STRING "DEB/RPM package release number")
if(NOT MSICONTROLLER_PACKAGE_RELEASE)
    set(MSICONTROLLER_PACKAGE_RELEASE "1")
endif()
string(REGEX REPLACE "[^A-Za-z0-9.+~]" "." MSICONTROLLER_PACKAGE_RELEASE "${MSICONTROLLER_PACKAGE_RELEASE}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
set(CPACK_PACKAGE_RELOCATABLE OFF)
set(CPACK_GENERATOR "DEB;RPM")
set(CPACK_SET_DESTDIR ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

include("${CMAKE_SOURCE_DIR}/cmake/packaging/BundleQtRuntime.cmake")

set(_packaging_script_dir "${CMAKE_BINARY_DIR}/packaging/scripts")
file(MAKE_DIRECTORY
    "${_packaging_script_dir}/deb"
    "${_packaging_script_dir}/rpm"
)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/packaging/scripts/deb/postinst.in"
    "${_packaging_script_dir}/deb/postinst"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/packaging/scripts/deb/prerm.in"
    "${_packaging_script_dir}/deb/prerm"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/packaging/scripts/rpm/postinstall.in"
    "${_packaging_script_dir}/rpm/postinstall"
    @ONLY
)
configure_file(
    "${CMAKE_SOURCE_DIR}/cmake/packaging/scripts/rpm/preuninstall.in"
    "${_packaging_script_dir}/rpm/preuninstall"
    @ONLY
)
execute_process(COMMAND chmod 0755
    "${_packaging_script_dir}/deb/postinst"
    "${_packaging_script_dir}/deb/prerm"
    "${_packaging_script_dir}/rpm/postinstall"
    "${_packaging_script_dir}/rpm/preuninstall"
)

set(CPACK_DEBIAN_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_RELEASE "${MSICONTROLLER_PACKAGE_RELEASE}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
if(MSICONTROLLER_BUNDLE_QT_RUNTIME)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "dkms, kmod, systemd, linux-headers-generic | linux-headers-amd64")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS
        "${CMAKE_INSTALL_PREFIX}/lib"
        "${CMAKE_INSTALL_PREFIX}/qt/lib"
    )
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "dkms, kmod, systemd, linux-headers-generic | linux-headers-amd64, qt6-qpa-plugins, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-shapes, qml6-module-qtquick-effects, qml6-module-qtcharts, qml6-module-qt-labs-platform, qml6-module-qt-labs-settings, qml6-module-qtcore"
    )
endif()
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${_packaging_script_dir}/deb/postinst;${_packaging_script_dir}/deb/prerm"
)

set(CPACK_RPM_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_RELEASE "${MSICONTROLLER_PACKAGE_RELEASE}")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)
if(MSICONTROLLER_BUNDLE_QT_RUNTIME)
    set(CPACK_RPM_PACKAGE_REQUIRES "dkms, kmod, systemd, kernel-devel")
else()
    set(CPACK_RPM_PACKAGE_REQUIRES
        "dkms, kmod, systemd, kernel-devel, qt6-qtbase-gui, qt6-qtdeclarative, qt6-qtcharts"
    )
endif()
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${_packaging_script_dir}/rpm/postinstall")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${_packaging_script_dir}/rpm/preuninstall")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    /etc
    /etc/dbus-1
    /etc/dbus-1/system.d
    /etc/modules-load.d
    /etc/xdg
    /etc/xdg/autostart
    /lib
    /lib/systemd
    /lib/systemd/system
    /opt
    /usr/src
)

include(CPack)
