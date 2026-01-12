#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/assignment_package/build/dev"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if command -v qmake-qt5 >/dev/null 2>&1; then
  QMAKE_BIN="qmake-qt5"
else
  QMAKE_BIN="qmake"
fi

"${QMAKE_BIN}" ../../miniMinecraft.pro
make -j"$(nproc)"
