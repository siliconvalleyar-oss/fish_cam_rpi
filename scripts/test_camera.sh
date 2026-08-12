#!/usr/bin/env bash
# =============================================================================
# scripts/test_camera.sh
#
# Tests that the OV5647 camera is detected and reports its state.
# Prints the device list, the legacy stack status and the V4L2 topology.
#
# Usage:
#   ./scripts/test_camera.sh
# =============================================================================

set -euo pipefail

echo "========================================================"
echo " 1. /dev/video* devices"
echo "========================================================"
if ls /dev/video* >/dev/null 2>&1; then
  ls -l /dev/video*
else
  echo "(none found)"
fi

echo
echo "========================================================"
echo " 2. Legacy camera stack (vcgencmd)"
echo "========================================================"
if command -v vcgencmd >/dev/null 2>&1; then
  vcgencmd get_camera
else
  echo "(vcgencmd not available)"
fi

echo
echo "========================================================"
echo " 3. V4L2 topology (v4l2-ctl)"
echo "========================================================"
if command -v v4l2-ctl >/dev/null 2>&1; then
  v4l2-ctl --list-devices || true
else
  echo "(v4l2-ctl not installed; run script_tools/install_dependencies.sh)"
fi

echo
echo "========================================================"
echo " 4. libcamera devices (informational)"
echo "========================================================"
if command -v libcamera-hello >/dev/null 2>&1; then
  libcamera-hello --list-cameras 2>/dev/null || true
else
  echo "(libcamera tools not installed)"
fi

echo
if command -v vcgencmd >/dev/null 2>&1 && \
     vcgencmd get_camera | grep -q "supported=1 detected=1"; then
  echo "RESULT: OV5647 camera OK"
  exit 0
else
  echo "RESULT: camera not detected."
  echo "  - Verify the flex cable is fully seated."
  echo "  - Re-run script_tools/install_dependencies.sh and reboot."
  exit 1
fi
