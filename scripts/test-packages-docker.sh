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
    -v "${PROJECT_ROOT}:/work:ro" \
    -v "${PACKAGE_DIR}:/packages:ro" \
    -w /work \
    "${image}" \
    bash /work/scripts/test-package-install.sh /packages
}

log() {
  printf '\n==> %s\n' "$*"
}

[ -f "${PACKAGE_DIR}/msicontroller_amd64.deb" ] || \
  { printf 'error: missing %s\n' "${PACKAGE_DIR}/msicontroller_amd64.deb" >&2; exit 1; }
[ -f "${PACKAGE_DIR}/msicontroller_x86_64.rpm" ] || \
  { printf 'error: missing %s\n' "${PACKAGE_DIR}/msicontroller_x86_64.rpm" >&2; exit 1; }

for image in "${DEB_IMAGES[@]}"; do
  run_image "${image}"
done

for image in "${RPM_IMAGES[@]}"; do
  run_image "${image}"
done
