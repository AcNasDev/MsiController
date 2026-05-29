#!/usr/bin/env bash
set -euo pipefail

QT_VERSION="${MSICONTROLLER_QT_VERSION:-6.11.1}"
QT_ARCH="${MSICONTROLLER_QT_ARCH:-linux_gcc_64}"
QT_OUTPUT_DIR="${MSICONTROLLER_QT_OUTPUT_DIR:-/opt/Qt}"
AQT_VERSION="${MSICONTROLLER_AQT_VERSION:-}"

if [[ -n "${MSICONTROLLER_HTTP_PROXY:-}" ]]; then
  export http_proxy="${MSICONTROLLER_HTTP_PROXY}"
  export https_proxy="${MSICONTROLLER_HTTP_PROXY}"
  export HTTP_PROXY="${MSICONTROLLER_HTTP_PROXY}"
  export HTTPS_PROXY="${MSICONTROLLER_HTTP_PROXY}"
fi

python3 -m pip install --no-cache-dir --upgrade pip setuptools wheel
if [[ -n "${AQT_VERSION}" ]]; then
  python3 -m pip install --no-cache-dir "aqtinstall==${AQT_VERSION}"
else
  python3 -m pip install --no-cache-dir aqtinstall
fi

aqt install-qt \
  --outputdir "${QT_OUTPUT_DIR}" \
  linux desktop "${QT_VERSION}" "${QT_ARCH}" \
  --modules qtcharts
