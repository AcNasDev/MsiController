#!/usr/bin/env bash
set -euo pipefail

PACKAGE_DIR="${1:-packages}"
PACKAGE_DIR="$(cd "${PACKAGE_DIR}" && pwd)"
DEB_PACKAGE="${MSICONTROLLER_TEST_DEB:-${PACKAGE_DIR}/msicontroller_amd64.deb}"
RPM_PACKAGE="${MSICONTROLLER_TEST_RPM:-${PACKAGE_DIR}/msicontroller_x86_64.rpm}"
export MSICONTROLLER_SKIP_DKMS="${MSICONTROLLER_SKIP_DKMS:-1}"

log() {
  printf '\n==> %s\n' "$*"
}

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

require_file() {
  [ -e "$1" ] || fail "missing $1"
}

require_executable() {
  [ -x "$1" ] || fail "missing executable $1"
}

require_desktop_entry() {
  local file="$1"
  local key="$2"
  local value="$3"

  grep -Fx "${key}=${value}" "${file}" >/dev/null || \
    fail "${file} does not contain ${key}=${value}"
}

validate_layout() {
  log "Validating installed layout"

  require_executable /opt/msicontroller/bin/MsiControlCenterClient
  require_executable /opt/msicontroller/bin/MsiControlCenterService
  require_executable /opt/msicontroller/bin/msicontroller-client
  require_executable /opt/msicontroller/bin/msicontroller-service
  require_file /opt/msicontroller/lib/libhelper.so
  require_file /opt/msicontroller/qt/lib/libQt6Core.so.6
  require_file /usr/share/applications/msi-control-center.desktop
  require_file /usr/share/icons/hicolor/scalable/apps/msi-control-center.svg
  require_file /etc/xdg/autostart/msi-control-center-autostart.desktop
  require_file /etc/dbus-1/system.d/msi-ec-service.conf
  require_file /lib/systemd/system/msi-ec-service.service

  require_desktop_entry /usr/share/applications/msi-control-center.desktop \
    Exec /opt/msicontroller/bin/msicontroller-client
  require_desktop_entry /etc/xdg/autostart/msi-control-center-autostart.desktop \
    Exec /opt/msicontroller/bin/msicontroller-client

  find /usr/src -maxdepth 1 -type d -name 'msiecmodule-*' | grep -q . || \
    fail "DKMS source directory was not installed under /usr/src"
}

validate_runtime_links() {
  log "Checking runtime library links"

  local ldd_output
  ldd_output="$(
    LD_LIBRARY_PATH=/opt/msicontroller/lib:/opt/msicontroller/qt/lib \
      ldd /opt/msicontroller/bin/MsiControlCenterClient
  )"
  printf '%s\n' "${ldd_output}"

  if printf '%s\n' "${ldd_output}" | grep -q 'not found'; then
    fail "client has unresolved shared libraries"
  fi
}

validate_desktop_file() {
  if command -v desktop-file-validate >/dev/null 2>&1; then
    log "Validating desktop file"
    desktop-file-validate /usr/share/applications/msi-control-center.desktop
  fi
}

test_deb_install() {
  [ -f "${DEB_PACKAGE}" ] || fail "DEB package not found: ${DEB_PACKAGE}"

  log "Installing DEB package on $(. /etc/os-release && printf '%s %s' "${ID}" "${VERSION_ID:-}")"
  apt-get update
  DEBIAN_FRONTEND=noninteractive \
  MSICONTROLLER_SKIP_DKMS=1 \
    apt-get install -y --no-install-recommends "${DEB_PACKAGE}"

  dpkg-query -W -f='${Package} ${Version} ${Architecture}\n' msicontroller
  validate_layout
  validate_runtime_links
  validate_desktop_file

  log "Removing DEB package"
  DEBIAN_FRONTEND=noninteractive \
  MSICONTROLLER_SKIP_DKMS=1 \
    apt-get remove -y msicontroller
}

test_rpm_install() {
  [ -f "${RPM_PACKAGE}" ] || fail "RPM package not found: ${RPM_PACKAGE}"

  log "Installing RPM package on $(. /etc/os-release && printf '%s %s' "${ID}" "${VERSION_ID:-}")"
  dnf install -y --setopt=install_weak_deps=False "${RPM_PACKAGE}"

  rpm -q msicontroller
  validate_layout
  validate_runtime_links
  validate_desktop_file

  log "Removing RPM package"
  rpm -e msicontroller
}

if command -v apt-get >/dev/null 2>&1; then
  test_deb_install
elif command -v dnf >/dev/null 2>&1; then
  test_rpm_install
else
  fail "unsupported package manager; expected apt-get or dnf"
fi
