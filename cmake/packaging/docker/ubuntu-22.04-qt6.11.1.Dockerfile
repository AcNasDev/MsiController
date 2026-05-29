# syntax=docker/dockerfile:1.7
FROM ubuntu:22.04

ARG QT_VERSION=6.11.1
ARG QT_ARCH=linux_gcc_64

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=secret,id=msicontroller_http_proxy \
    set -eu; \
    if [ -s /run/secrets/msicontroller_http_proxy ]; then \
      proxy="$(cat /run/secrets/msicontroller_http_proxy)"; \
      export http_proxy="${proxy}" https_proxy="${proxy}" HTTP_PROXY="${proxy}" HTTPS_PROXY="${proxy}"; \
    fi; \
    apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    dpkg-dev \
    file \
    git \
    libdbus-1-dev \
    libegl1-mesa-dev \
    libfontconfig1-dev \
    libfreetype-dev \
    libgl1-mesa-dev \
    libsystemd-dev \
    libxcb-cursor0 \
    libxcb-icccm4 \
    libxcb-image0 \
    libxcb-keysyms1 \
    libxcb-render-util0 \
    libxcb-render0 \
    libxcb-shape0 \
    libxcb-util1 \
    libxkbcommon-dev \
    libxkbcommon-x11-0 \
    ninja-build \
    pkg-config \
    python3 \
    python3-pip \
    rpm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /work

COPY scripts/install-qt-aqt.sh /tmp/install-qt-aqt.sh
RUN --mount=type=secret,id=msicontroller_http_proxy \
    set -eu; \
    if [ -s /run/secrets/msicontroller_http_proxy ]; then \
      MSICONTROLLER_HTTP_PROXY="$(cat /run/secrets/msicontroller_http_proxy)"; \
      export MSICONTROLLER_HTTP_PROXY; \
    fi; \
    chmod +x /tmp/install-qt-aqt.sh; \
    MSICONTROLLER_QT_VERSION="${QT_VERSION}" \
      MSICONTROLLER_QT_ARCH="${QT_ARCH}" \
      /tmp/install-qt-aqt.sh; \
    rm -rf /root/.cache/pip /tmp/install-qt-aqt.sh

ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8
ENV MSICONTROLLER_QT_VERSION=${QT_VERSION}
ENV MSICONTROLLER_QT_HOST_DIR=/opt/Qt/${QT_VERSION}/gcc_64
ENV PATH=/opt/Qt/${QT_VERSION}/gcc_64/bin:${PATH}
ENV CMAKE_PREFIX_PATH=/opt/Qt/${QT_VERSION}/gcc_64
