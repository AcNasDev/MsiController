#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_DIR="${MSICONTROLLER_PACKAGE_OUTPUT_DIR:-${PROJECT_ROOT}/packages}"

DEB_IMAGES=(
  ubuntu:22.04
  ubuntu:24.04
  ubuntu:26.04
  debian:12
)

RPM_IMAGES=(
  fedora:latest
)

run_image() {
  local image="$1"
  log "Testing packages in ${image}"
  docker run --rm \
    -e MSICONTROLLER_SKIP_DKMS=1 \
    -e MSICONTROLLER_DISABLE_REPOSITORY_SETUP=1 \
    -v "${PROJECT_ROOT}:/work:ro" \
    -v "${PACKAGE_DIR}:/packages:ro" \
    -w /work \
    "${image}" \
    bash /work/scripts/test-package-install.sh /packages
}

log() {
  printf '\n==> %s\n' "$*"
}

shopt -s nullglob
deb_packages=("${PACKAGE_DIR}"/msicontroller-[0-9]*-*-*.deb)
rpm_packages=("${PACKAGE_DIR}"/msicontroller-[0-9]*-*-*.rpm)
[[ ${#deb_packages[@]} -eq 1 && ${#rpm_packages[@]} -eq 1 ]] || {
  printf 'error: expected one versioned DEB and one versioned RPM package in %s\n' "${PACKAGE_DIR}" >&2
  exit 1
}

for image in "${DEB_IMAGES[@]}"; do
  run_image "${image}"
done

for image in "${RPM_IMAGES[@]}"; do
  run_image "${image}"
done
