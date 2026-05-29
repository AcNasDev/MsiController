#!/usr/bin/env bash
set -euo pipefail

QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_HOST_DIR="${MSICONTROLLER_QT_HOST_DIR:-/opt/Qt/${QT_VERSION}/gcc_64}"
PACKAGE_PREFIX="${MSICONTROLLER_PACKAGE_PREFIX:-/opt/msicontroller}"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_OUTPUT_DIR="${MSICONTROLLER_PACKAGE_OUTPUT_DIR:-${PROJECT_ROOT}/packages}"

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

rm -rf "${PACKAGE_OUTPUT_DIR}"
mkdir -p "${PACKAGE_OUTPUT_DIR}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PACKAGE_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${QT_HOST_DIR}" \
  -DMSICONTROLLER_BUNDLE_QT_RUNTIME=ON \
  -DMSICONTROLLER_BUILD_KERNEL_MODULE=OFF \
  -DMSICONTROLLER_INSTALL_DKMS=ON

cmake --build "${BUILD_DIR}" --parallel

(
  cd "${BUILD_DIR}"
  cpack -G DEB
  cpack -G RPM
)

cp "${BUILD_DIR}"/*.deb "${BUILD_DIR}"/*.rpm "${PACKAGE_OUTPUT_DIR}/"
