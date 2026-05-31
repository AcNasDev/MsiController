#!/usr/bin/env bash
set -euo pipefail

PACKAGE_DIR="${1:-packages}"
PACKAGE_DIR="$(cd "${PACKAGE_DIR}" && pwd)"
DEB_PACKAGE="${MSICONTROLLER_PUBLISH_DEB:-${PACKAGE_DIR}/msicontroller_amd64.deb}"
RPM_PACKAGE="${MSICONTROLLER_PUBLISH_RPM:-${PACKAGE_DIR}/msicontroller_x86_64.rpm}"
DEBIAN_DISTRIBUTION="${MSICONTROLLER_FORGEJO_DEBIAN_DISTRIBUTION:-stable}"
DEBIAN_COMPONENT="${MSICONTROLLER_FORGEJO_DEBIAN_COMPONENT:-main}"
RPM_GROUP="${MSICONTROLLER_FORGEJO_RPM_GROUP:-}"

log() {
  printf '==> %s\n' "$*"
}

fail() {
  printf 'error: %s\n' "$*" >&2
  exit 1
}

require_file() {
  [ -f "$1" ] || fail "missing package file: $1"
}

forgejo_base_url() {
  if [[ -n "${MSICONTROLLER_FORGEJO_BASE_URL:-}" ]]; then
    printf '%s\n' "${MSICONTROLLER_FORGEJO_BASE_URL%/}"
    return
  fi

  if [[ -n "${FORGEJO_SERVER_URL:-}" ]]; then
    printf '%s\n' "${FORGEJO_SERVER_URL%/}"
    return
  fi

  if [[ -n "${FORGEJO_API_URL:-}" ]]; then
    local base="${FORGEJO_API_URL%/}"
    base="${base%/api/v1}"
    printf '%s\n' "${base%/}"
    return
  fi

  fail "set FORGEJO_API_URL, FORGEJO_SERVER_URL, or MSICONTROLLER_FORGEJO_BASE_URL"
}

package_owner() {
  if [[ -n "${MSICONTROLLER_FORGEJO_PACKAGE_OWNER:-}" ]]; then
    printf '%s\n' "${MSICONTROLLER_FORGEJO_PACKAGE_OWNER}"
    return
  fi

  if [[ -n "${FORGEJO_REPOSITORY:-}" ]]; then
    printf '%s\n' "${FORGEJO_REPOSITORY%%/*}"
    return
  fi

  fail "set FORGEJO_REPOSITORY or MSICONTROLLER_FORGEJO_PACKAGE_OWNER"
}

curl_auth=()
prepare_auth() {
  [[ -n "${FORGEJO_TOKEN:-}" ]] || fail "set FORGEJO_TOKEN"

  if [[ -n "${FORGEJO_ACTOR:-}" ]]; then
    curl_auth=(--user "${FORGEJO_ACTOR}:${FORGEJO_TOKEN}")
  else
    curl_auth=(-H "Authorization: token ${FORGEJO_TOKEN}")
  fi
}

delete_if_present() {
  local url="$1"
  curl -fsS -X DELETE "${curl_auth[@]}" "$url" >/dev/null 2>&1 || true
}

upload_file() {
  local file="$1"
  local url="$2"
  local response status

  response="$(mktemp)"
  status="$(curl -sS -o "$response" -w '%{http_code}' \
    "${curl_auth[@]}" \
    --upload-file "$file" \
    "$url" || true)"

  case "$status" in
    200|201|204)
      rm -f "$response"
      ;;
    409)
      printf 'Package already exists in Forgejo registry: %s\n' "$(basename "$file")" >&2
      rm -f "$response"
      ;;
    *)
      cat "$response" >&2 || true
      rm -f "$response"
      fail "Forgejo upload failed for $(basename "$file") with HTTP $status"
      ;;
  esac
}

publish_deb() {
  local file="$1"
  local package_name package_version package_arch delete_url upload_url

  command -v dpkg-deb >/dev/null 2>&1 || fail "dpkg-deb is required to publish DEB packages"

  package_name="$(dpkg-deb -f "$file" Package)"
  package_version="$(dpkg-deb -f "$file" Version)"
  package_arch="$(dpkg-deb -f "$file" Architecture)"

  delete_url="${package_api}/debian/pool/${DEBIAN_DISTRIBUTION}/${DEBIAN_COMPONENT}/${package_name}/${package_version}/${package_arch}"
  upload_url="${package_api}/debian/pool/${DEBIAN_DISTRIBUTION}/${DEBIAN_COMPONENT}/upload"

  log "Publishing DEB ${package_name} ${package_version} ${package_arch} to ${DEBIAN_DISTRIBUTION}/${DEBIAN_COMPONENT}"
  delete_if_present "$delete_url"
  upload_file "$file" "$upload_url"
}

publish_rpm() {
  local file="$1"
  local package_name package_version package_arch rpm_base delete_url upload_url

  command -v rpm >/dev/null 2>&1 || fail "rpm is required to publish RPM packages"

  package_name="$(rpm -qp --qf '%{NAME}' "$file")"
  package_version="$(rpm -qp --qf '%{VERSION}-%{RELEASE}' "$file")"
  package_arch="$(rpm -qp --qf '%{ARCH}' "$file")"

  rpm_base="${package_api}/rpm"
  if [[ -n "$RPM_GROUP" ]]; then
    rpm_base="${rpm_base}/${RPM_GROUP}"
  fi

  delete_url="${rpm_base}/package/${package_name}/${package_version}/${package_arch}"
  upload_url="${rpm_base}/upload"

  log "Publishing RPM ${package_name} ${package_version} ${package_arch}"
  delete_if_present "$delete_url"
  upload_file "$file" "$upload_url"
}

require_file "$DEB_PACKAGE"
require_file "$RPM_PACKAGE"
prepare_auth

forgejo_base="$(forgejo_base_url)"
owner="$(package_owner)"
package_api="${forgejo_base}/api/packages/${owner}"

publish_deb "$DEB_PACKAGE"
publish_rpm "$RPM_PACKAGE"
