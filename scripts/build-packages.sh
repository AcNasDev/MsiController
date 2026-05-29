#!/usr/bin/env bash
set -euo pipefail

QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_HOST_DIR="${MSICONTROLLER_QT_HOST_DIR:-/opt/Qt/${QT_VERSION}/gcc_64}"
PACKAGE_PREFIX="${MSICONTROLLER_PACKAGE_PREFIX:-/opt/msicontroller}"

export PATH="${QT_HOST_DIR}/bin:${PATH}"
export CMAKE_PREFIX_PATH="${QT_HOST_DIR}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"

rm -rf build-package packages

cmake -S . -B build-package -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${PACKAGE_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${QT_HOST_DIR}" \
  -DMSICONTROLLER_BUNDLE_QT_RUNTIME=ON \
  -DMSICONTROLLER_BUILD_KERNEL_MODULE=OFF \
  -DMSICONTROLLER_INSTALL_DKMS=ON

cmake --build build-package --parallel

cd build-package
cpack -G DEB
cpack -G RPM

mkdir -p ../packages
cp ./*.deb ./*.rpm ../packages/
