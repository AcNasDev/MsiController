#!/usr/bin/env bash
set -euo pipefail

IMAGE="${MSICONTROLLER_PACKAGE_IMAGE:-msicontroller-package:ubuntu-22.04-qt6.11.1}"
DOCKERFILE="${MSICONTROLLER_PACKAGE_DOCKERFILE:-cmake/packaging/docker/ubuntu-22.04-qt6.11.1.Dockerfile}"
PACKAGE_PREFIX="${MSICONTROLLER_PACKAGE_PREFIX:-/opt/msicontroller}"
QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_ARCH="${MSICONTROLLER_QT_ARCH:-linux_gcc_64}"

if [[ -n "${MSICONTROLLER_HTTP_PROXY:-}" ]]; then
  export http_proxy="${MSICONTROLLER_HTTP_PROXY}"
  export https_proxy="${MSICONTROLLER_HTTP_PROXY}"
  export HTTP_PROXY="${MSICONTROLLER_HTTP_PROXY}"
  export HTTPS_PROXY="${MSICONTROLLER_HTTP_PROXY}"
fi

RUN_ENV_FILE="$(mktemp)"
trap 'rm -f "${RUN_ENV_FILE}"' EXIT

{
  printf 'HOME=/tmp\n'
  printf 'http_proxy=%s\n' "${http_proxy:-}"
  printf 'https_proxy=%s\n' "${https_proxy:-}"
  printf 'HTTP_PROXY=%s\n' "${HTTP_PROXY:-}"
  printf 'HTTPS_PROXY=%s\n' "${HTTPS_PROXY:-}"
} > "${RUN_ENV_FILE}"
chmod 0600 "${RUN_ENV_FILE}"

docker_build_args=(
  --build-arg QT_VERSION="${QT_VERSION}" \
  --build-arg QT_ARCH="${QT_ARCH}" \
  -f "${DOCKERFILE}" \
  -t "${IMAGE}" \
)

if [[ -n "${MSICONTROLLER_HTTP_PROXY:-}" ]]; then
  docker_build_args+=(--secret id=msicontroller_http_proxy,env=MSICONTROLLER_HTTP_PROXY)
fi

DOCKER_BUILDKIT="${DOCKER_BUILDKIT:-1}" docker build "${docker_build_args[@]}" .

docker run --rm \
  --user "$(id -u):$(id -g)" \
  --env-file "${RUN_ENV_FILE}" \
  -v "${PWD}:/work" \
  -w /work \
  "${IMAGE}" \
  bash -lc "MSICONTROLLER_PACKAGE_PREFIX='${PACKAGE_PREFIX}' MSICONTROLLER_QT_VERSION='${QT_VERSION}' ./scripts/build-packages.sh"
