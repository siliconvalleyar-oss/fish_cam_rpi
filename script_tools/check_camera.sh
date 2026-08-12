#!/usr/bin/env bash
# =============================================================================
# script_tools/check_camera.sh
#
# Diagnoses the OV5647 camera setup on Raspberry Pi OS before/after running
# fish_cam_rpi. Checks, in order:
#   1. /dev/video* devices present
#   2. Legacy MMAL stack (Bullseye and older): vcgencmd get_camera
#   3. libcamera stack (Bookworm and newer): rpicam-hello --list-cameras
#   4. /boot config.txt camera settings
#   5. A real capture through ./bin/fish_cam_rpi (uses the V4L2 compat layer
#      on Bookworm+, so it validates the whole runtime path)
#
# Usage:
#   ./script_tools/check_camera.sh
#
# Exit codes: 0 = everything OK, 1 = camera not usable.
# =============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BIN="${ROOT_DIR}/bin/fish_cam_rpi"
WORK_DIR="$(mktemp -d)"

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[1;33m'
NC=$'\033[0m'

PASS=0
WARN=0
FAIL=0

cleanup() {
  rm -rf "${WORK_DIR}"
}
trap cleanup EXIT

ok()    { PASS=$((PASS + 1)); echo "  [OK]   $*"; }
warn()  { WARN=$((WARN + 1)); echo "  [WARN] $*"; }
fail()  { FAIL=$((FAIL + 1)); echo "  [FAIL] $*"; }

has_legacy_stack() {
  [[ -f /opt/vc/lib/libmmal.so ]] || [[ -f /usr/lib/arm-linux-gnueabihf/libmmal.so ]] \
    || [[ -f /usr/include/interface/mmal/mmal.h ]]
}

echo "== Camera check: fish_cam_rpi =="

# --------------------------------------------------------------------------
# 1. /dev/video* devices
# --------------------------------------------------------------------------
echo "1. Device nodes"
mapfile -t DEVICES < <(ls /dev/video* 2>/dev/null)
if [[ "${#DEVICES[@]}" -gt 0 ]]; then
  ok "found: ${DEVICES[*]}"
  if [[ -e /dev/video0 ]]; then
    ok "found: /dev/video0"
  elif has_legacy_stack; then
    warn "no /dev/video0 (is the bcm2835-v4l2 driver loaded?)"
  else
    warn "no /dev/video0 yet; on Bookworm+ it appears when running with the"
    warn "libcamera v4l2-compat layer (LD_PRELOAD), see docs/INSTALL.md"
  fi
  if command -v v4l2-ctl >/dev/null 2>&1; then
    for dev in "${DEVICES[@]}"; do
      echo "     $(v4l2-ctl -d "${dev}" --info 2>/dev/null | head -3 | tr '\n' ' ')"
    done
  fi
else
  warn "no /dev/video* nodes (camera disabled or not detected)"
fi

# --------------------------------------------------------------------------
# 2. Legacy MMAL stack (Bullseye and older)
# --------------------------------------------------------------------------
echo "2. Camera stack"
if has_legacy_stack; then
  if command -v vcgencmd >/dev/null 2>&1; then
    REPORT="$(vcgencmd get_camera 2>/dev/null)"
    echo "     legacy stack: ${REPORT}"
    case "${REPORT}" in
      *"supported=1 detected=1"*) ok "legacy camera detected and enabled" ;;
      *"supported=1 detected=0"*) fail "legacy camera supported but NOT detected (check ribbon cable)" ;;
      *"supported=0"*) fail "legacy camera not supported (start_x=1 missing?)" ;;
      *) warn "unexpected vcgencmd output: ${REPORT}" ;;
    esac
  else
    warn "legacy stack present but vcgencmd not found"
  fi
else
  echo "     legacy MMAL stack not present (expected on Bookworm+)."
  if command -v rpicam-hello >/dev/null 2>&1; then
    if rpicam-hello --list-cameras 2>/dev/null | grep -q "0 :"; then
      ok "libcamera detects a camera"
    else
      fail "rpicam-hello reports no cameras (check ribbon cable / dtoverlay)"
    fi
  else
    warn "rpicam-hello not installed (install libcamera-apps on Bookworm+)"
  fi
fi

# --------------------------------------------------------------------------
# 3. /boot config.txt camera settings
# --------------------------------------------------------------------------
echo "3. Camera config (/boot*config.txt)"
CONFIG_TXT="/boot/config.txt"
[[ -f /boot/firmware/config.txt ]] && CONFIG_TXT="/boot/firmware/config.txt"
if [[ -f "${CONFIG_TXT}" ]]; then
  if grep -qE "^[[:space:]]*(start_x=1|camera_auto_detect=0)" "${CONFIG_TXT}" 2>/dev/null; then
    ok "legacy camera lines present in ${CONFIG_TXT}"
  else
    warn "no legacy camera lines in ${CONFIG_TXT} (libcamera mode, OK on Bookworm+)"
  fi
else
  warn "no ${CONFIG_TXT} found"
fi

# --------------------------------------------------------------------------
# 4. Real capture with fish_cam_rpi
# --------------------------------------------------------------------------
echo "4. Live capture"
if [[ ! -x "${BIN}" ]]; then
  warn "binary not built yet; run: make"
  echo "   (skipping live capture)"
else
  PRELOAD=()
  if ! has_legacy_stack && [[ -d /usr/libexec ]]; then
    COMPAT="$(ls /usr/libexec/*/libcamera/v4l2-compat.so 2>/dev/null | head -1)"
    [[ -n "${COMPAT}" ]] && PRELOAD=(env LD_PRELOAD="${COMPAT}")
  fi
  if "${PRELOAD[@]}" "${BIN}" --capture --output-dir "${WORK_DIR}" >/dev/null 2>&1; then
    IMG="$(ls "${WORK_DIR}"/*.jpg "${WORK_DIR}"/*.png 2>/dev/null | head -1)"
    if [[ -n "${IMG}" && -s "${IMG}" ]]; then
      ok "captured: ${IMG##*/} ($(stat -c%s "${IMG}") bytes)"
    else
      ok "capture succeeded"
    fi
  else
    fail "capture failed (see: make && ./bin/fish_cam_rpi --capture)"
  fi
fi

# --------------------------------------------------------------------------
# Summary
# --------------------------------------------------------------------------
echo
echo "== Result: ${PASS} ok, ${WARN} warnings, ${FAIL} failures =="
if [[ "${FAIL}" -gt 0 ]]; then
  echo "Camera is NOT usable yet. See docs/INSTALL.md (camera wiring,"
  echo "raspi-config > Interface Options > I1 Legacy Camera, reboot)."
  exit 1
fi
[[ "${PASS}" -gt 0 ]] && exit 0
echo "No camera checks passed; enable the camera and reboot, then re-run."
exit 1
