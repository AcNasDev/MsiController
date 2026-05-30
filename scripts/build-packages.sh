#!/usr/bin/env bash
set -euo pipefail

QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_HOST_DIR="${MSICONTROLLER_QT_HOST_DIR:-/opt/Qt/${QT_VERSION}/gcc_64}"
PACKAGE_PREFIX="${MSICONTROLLER_PACKAGE_PREFIX:-/opt/msicontroller}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_OUTPUT_DIR="${MSICONTROLLER_PACKAGE_OUTPUT_DIR:-${PROJECT_ROOT}/packages}"
if [[ -n "${MSICONTROLLER_PACKAGE_RELEASE:-}" ]]; then
  PACKAGE_RELEASE="${MSICONTROLLER_PACKAGE_RELEASE}"
elif git -C "${PROJECT_ROOT}" describe --tags --exact-match >/dev/null 2>&1; then
  PACKAGE_RELEASE="1"
else
  PACKAGE_RELEASE="$(date -u +%Y%m%d%H%M%S)"
fi
DEB_ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
RPM_ARCH="$(rpm --eval '%{_target_cpu}' 2>/dev/null || uname -m)"
DEB_FILE_NAME="${MSICONTROLLER_DEB_FILE_NAME:-msicontroller_${DEB_ARCH}.deb}"
RPM_FILE_NAME="${MSICONTROLLER_RPM_FILE_NAME:-msicontroller_${RPM_ARCH}.rpm}"

export PATH="${QT_HOST_DIR}/bin:${PATH}"
export CMAKE_PREFIX_PATH="${QT_HOST_DIR}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"

if [[ -n "${MSICONTROLLER_PACKAGE_BUILD_DIR:-}" ]]; then
  BUILD_DIR="${MSICONTROLLER_PACKAGE_BUILD_DIR}"
  rm -rf "${BUILD_DIR}"
  CLEAN_BUILD_DIR=0
else
  BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/msicontroller-package-build.XXXXXX")"
  CLEAN_BUILD_DIR=1
fi

cleanup() {
  if [[ "${CLEAN_BUILD_DIR}" == "1" ]]; then
    rm -rf "${BUILD_DIR}"
  fi
}
trap cleanup EXIT

mkdir -p "${PACKAGE_OUTPUT_DIR}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PACKAGE_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${QT_HOST_DIR}" \
  -DMSICONTROLLER_BUNDLE_QT_RUNTIME=ON \
  -DMSICONTROLLER_BUILD_KERNEL_MODULE=OFF \
  -DMSICONTROLLER_INSTALL_DKMS=ON \
  -DMSICONTROLLER_PACKAGE_RELEASE="${PACKAGE_RELEASE}"

cmake --build "${BUILD_DIR}" --parallel

(
  cd "${BUILD_DIR}"
  cpack -G DEB
  cpack -G RPM
)

DEB_PACKAGE="$(find "${BUILD_DIR}" -maxdepth 1 -type f -name '*.deb' -print -quit)"
RPM_PACKAGE="$(find "${BUILD_DIR}" -maxdepth 1 -type f -name '*.rpm' -print -quit)"

install -m 0644 "${DEB_PACKAGE}" "${PACKAGE_OUTPUT_DIR}/${DEB_FILE_NAME}.tmp"
install -m 0644 "${RPM_PACKAGE}" "${PACKAGE_OUTPUT_DIR}/${RPM_FILE_NAME}.tmp"
mv -f "${PACKAGE_OUTPUT_DIR}/${DEB_FILE_NAME}.tmp" "${PACKAGE_OUTPUT_DIR}/${DEB_FILE_NAME}"
mv -f "${PACKAGE_OUTPUT_DIR}/${RPM_FILE_NAME}.tmp" "${PACKAGE_OUTPUT_DIR}/${RPM_FILE_NAME}"
