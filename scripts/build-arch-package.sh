#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PACKAGE_OUTPUT_DIR="${MSICONTROLLER_PACKAGE_OUTPUT_DIR:-${PROJECT_ROOT}/packages}"
VERSION="${MSICONTROLLER_ARCH_VERSION:-}"
GIT_ROOT="$(git -C "${PROJECT_ROOT}" rev-parse --show-toplevel 2>/dev/null || true)"
if [[ -z "${VERSION}" ]]; then
  if [[ "${GIT_ROOT}" == "${PROJECT_ROOT}" ]]; then
    TAG="$(git -C "${PROJECT_ROOT}" describe --tags --match 'v[0-9]*' --abbrev=0)"
    if [[ "${TAG}" =~ ^v([0-9]+\.[0-9]+\.[0-9]+)(-.+)?$ ]]; then
      VERSION="${BASH_REMATCH[1]}"
    fi
  elif [[ "${PROJECT_ROOT##*/}" =~ ^[Mm]si[Cc]ontroller[-_][vV]?([0-9]+\.[0-9]+\.[0-9]+)$ ]]; then
    VERSION="${BASH_REMATCH[1]}"
  fi
fi
if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
  printf 'error: invalid Arch package version: %s\n' "${VERSION}" >&2
  exit 1
fi

source "${PROJECT_ROOT}/scripts/package-release.sh"
RELEASE="$(msicontroller_package_release "${PROJECT_ROOT}")"

if [[ "$(id -u)" == 0 ]]; then
  printf 'error: makepkg must run as an unprivileged user\n' >&2
  exit 1
fi
command -v makepkg >/dev/null || { printf 'error: makepkg is required\n' >&2; exit 1; }
QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_OUTPUT_DIR="${MSICONTROLLER_QT_OUTPUT_DIR:-${XDG_CACHE_HOME:-${HOME}/.cache}/msicontroller/Qt}"
qt_installation_complete() {
  [[ -x "$1/bin/qmake" &&
     -f "$1/lib/cmake/Qt6/Qt6Config.cmake" &&
     -f "$1/lib/cmake/Qt6Charts/Qt6ChartsConfig.cmake" &&
     -f "$1/lib/cmake/Qt6TaskTree/Qt6TaskTreeConfig.cmake" ]]
}
if [[ -n "${MSICONTROLLER_QT_HOST_DIR:-}" ]]; then
  QT_HOST_DIR="${MSICONTROLLER_QT_HOST_DIR}"
elif qt_installation_complete "/opt/Qt/${QT_VERSION}/gcc_64"; then
  QT_HOST_DIR="/opt/Qt/${QT_VERSION}/gcc_64"
else
  QT_HOST_DIR="${QT_OUTPUT_DIR}/${QT_VERSION}/gcc_64"
fi
if ! qt_installation_complete "${QT_HOST_DIR}"; then
  if [[ -n "${MSICONTROLLER_QT_HOST_DIR:-}" ]]; then
    printf 'error: Qt, Charts, or TaskTree missing in %s\n' "${QT_HOST_DIR}" >&2
    exit 1
  fi
  MSICONTROLLER_QT_VERSION="${QT_VERSION}" MSICONTROLLER_QT_OUTPUT_DIR="${QT_OUTPUT_DIR}" \
    "${PROJECT_ROOT}/scripts/install-qt-aqt.sh"
  qt_installation_complete "${QT_HOST_DIR}" || {
    printf 'error: incomplete Qt installation in %s\n' "${QT_HOST_DIR}" >&2
    exit 1
  }
fi
if [[ "$("${QT_HOST_DIR}/bin/qmake" -query QT_VERSION)" != "${QT_VERSION}" ]]; then
  printf 'error: expected Qt %s in %s\n' "${QT_VERSION}" "${QT_HOST_DIR}" >&2
  exit 1
fi
BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/msicontroller-arch-build.XXXXXX")"
trap 'rm -rf "${BUILD_DIR}"' EXIT
ARCHIVE="MsiController-${VERSION}.tar.gz"
if [[ "${GIT_ROOT}" == "${PROJECT_ROOT}" ]]; then
  while IFS= read -r -d '' source_file; do
    [[ "${source_file}" == .serena/* ]] && continue
    printf '%s\0' "${source_file}"
  done < <(git -C "${PROJECT_ROOT}" ls-files -z --cached --others --exclude-standard) | \
    tar -C "${PROJECT_ROOT}" --null -T - \
      --transform="s#^#MsiController-${VERSION}/#" -cf - | gzip -n > "${BUILD_DIR}/${ARCHIVE}"
else
  tar -C "${PROJECT_ROOT}" --exclude=.git --exclude=build --exclude=packages \
    --transform="s#^\./#MsiController-${VERSION}/#" -cf - . | gzip -n > "${BUILD_DIR}/${ARCHIVE}"
fi
SHA256="$(sha256sum "${BUILD_DIR}/${ARCHIVE}" | cut -d' ' -f1)"
sed -e "s/@VERSION@/${VERSION}/g" \
    -e "s/@RELEASE@/${RELEASE}/g" \
    -e "s/@SHA256@/${SHA256}/g" \
    "${PROJECT_ROOT}/cmake/packaging/arch/PKGBUILD.in" > "${BUILD_DIR}/PKGBUILD"
cp "${PROJECT_ROOT}/cmake/packaging/arch/msicontroller.install" "${BUILD_DIR}/msicontroller.install"

(cd "${BUILD_DIR}" && MSICONTROLLER_QT_HOST_DIR="${QT_HOST_DIR}" makepkg --syncdeps --noconfirm --clean --force)
mkdir -p "${PACKAGE_OUTPUT_DIR}"
PACKAGE_FILE="msicontroller-${VERSION}-${RELEASE}-x86_64.pkg.tar.zst"
install -m 0644 "${BUILD_DIR}/${PACKAGE_FILE}" "${PACKAGE_OUTPUT_DIR}/${PACKAGE_FILE}.tmp"
mv -f "${PACKAGE_OUTPUT_DIR}/${PACKAGE_FILE}.tmp" "${PACKAGE_OUTPUT_DIR}/${PACKAGE_FILE}"
printf 'Built %s\n' "${PACKAGE_OUTPUT_DIR}/${PACKAGE_FILE}"
