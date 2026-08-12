#!/usr/bin/env bash
# =============================================================================
# tests/integration_tests/test_integration.sh
#
# Integration tests for fish_cam_rpi. These require a working camera on the
# Raspberry Pi (the legacy camera stack enabled).
#
# Usage:
#   ./tests/integration_tests/test_integration.sh
#
# Exit codes: 0 = all tests passed, 1 = a test failed.
# =============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN="${ROOT_DIR}/bin/fish_cam_rpi"
WORK_DIR="$(mktemp -d)"

PASS=0
FAIL=0

cleanup() {
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

report() {
  if [[ "$1" -eq 0 ]]; then
    PASS=$((PASS + 1))
    echo "  [PASS] $2"
  else
    FAIL=$((FAIL + 1))
    echo "  [FAIL] $2"
  fi
}

if [[ ! -x "${BIN}" ]]; then
  echo "Building fish_cam_rpi first..."
  ( cd "${ROOT_DIR}" && make ) || { echo "Build failed"; exit 1; }
fi

echo "== Integration test: fish_cam_rpi =="

# 1. Help exits cleanly
"${BIN}" --help >/dev/null 2>&1
report $? "help"

# 2. Demo mode produces 3 images without a camera
"${BIN}" --demo --output-dir "${WORK_DIR}" >/dev/null 2>&1
COUNT=$(ls "${WORK_DIR}"/demo_* 2>/dev/null | wc -l)
[[ "${COUNT}" -eq 3 ]]
report $? "demo produced 3 images (got ${COUNT})"

# 3. Invalid format falls back to JPG
"${BIN}" --demo --format=bmp --output-dir "${WORK_DIR}" >/dev/null 2>&1
report $? "invalid format fallback"

# 4. Camera capture (requires hardware)
if "${BIN}" --capture --output-dir "${WORK_DIR}" >/dev/null 2>&1; then
  report 0 "single capture"
else
  report 1 "single capture (camera unavailable?)"
fi

echo
echo "== Results: ${PASS} passed, ${FAIL} failed =="
[[ "${FAIL}" -eq 0 ]]
