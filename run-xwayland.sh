#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/assignment_package/build/dev"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "Build directory not found: ${BUILD_DIR}" >&2
  echo "Run ./build.sh first." >&2
  exit 1
fi

export QT_QPA_PLATFORM=xcb
unset WAYLAND_DISPLAY

if [[ -z "${DISPLAY:-}" ]]; then
  if [[ -S /tmp/.X11-unix/X0 ]]; then
    export DISPLAY=:0
  elif [[ -S /tmp/.X11-unix/X1 ]]; then
    export DISPLAY=:1
  else
    echo "DISPLAY is not set and no X11 socket found in /tmp/.X11-unix." >&2
    exit 1
  fi
fi

cd "${BUILD_DIR}"
exec ./MiniMinecraft
