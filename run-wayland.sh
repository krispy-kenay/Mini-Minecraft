#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/assignment_package/build/dev"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "Build directory not found: ${BUILD_DIR}" >&2
  echo "Run ./build.sh first." >&2
  exit 1
fi

export QT_QPA_PLATFORM=wayland
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"

if [[ ! -S "${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" ]]; then
  echo "Wayland socket not found: ${XDG_RUNTIME_DIR}/${WAYLAND_DISPLAY}" >&2
  echo "Make sure the container has Wayland passthrough enabled." >&2
  exit 1
fi

cd "${BUILD_DIR}"
exec ./MiniMinecraft
