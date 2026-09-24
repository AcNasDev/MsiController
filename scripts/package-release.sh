#!/usr/bin/env bash

# Print the same numeric package release for DEB, RPM, and Arch builds.
msicontroller_package_release() {
  local project_root="$1"
  local override="${MSICONTROLLER_PACKAGE_RELEASE:-}"
  local git_root tag distance

  if [[ -n "${override}" ]]; then
    if [[ ! "${override}" =~ ^[1-9][0-9]*$ ]]; then
      printf 'error: invalid package release: %s\n' "${override}" >&2
      return 1
    fi
    printf '%s\n' "${override}"
    return
  fi

  git_root="$(git -C "${project_root}" rev-parse --show-toplevel 2>/dev/null || true)"
  if [[ "${git_root}" != "${project_root}" ]]; then
    printf '1\n'
    return
  fi

  tag="$(git -C "${project_root}" describe --tags --match 'v[0-9]*' --abbrev=0 2>/dev/null || true)"
  if [[ -n "${tag}" ]]; then
    distance="$(git -C "${project_root}" rev-list --count "${tag}..HEAD")"
  else
    distance="$(git -C "${project_root}" rev-list --count HEAD)"
  fi
  printf '%s\n' "$((distance + 1))"
}
