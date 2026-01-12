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

if [[ -z "${DISPLAY:-}" ]]; then
  echo "DISPLAY is not set. X11 forwarding is not available." >&2
  exit 1
fi

cd "${BUILD_DIR}"
exec ./MiniMinecraft
