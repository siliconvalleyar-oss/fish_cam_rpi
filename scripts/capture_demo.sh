#!/usr/bin/env bash
# =============================================================================
# scripts/capture_demo.sh
#
# Runs a short demonstration of the capture pipeline:
#   - a single capture
#   - a burst of 3 images
#   - a 2-second timelapse with 3 frames
#
# Usage:
#   ./scripts/capture_demo.sh [output_dir]
# =============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"
OUTPUT_DIR="${1:-${ROOT_DIR}/captures}"

cd "${ROOT_DIR}"

if [[ ! -x ./bin/fish_cam_rpi ]]; then
  echo "Binary not found; building first..."
  make
fi

mkdir -p "${OUTPUT_DIR}"

echo "==> Single capture"
./bin/fish_cam_rpi --capture --output-dir "${OUTPUT_DIR}"

echo "==> Burst of 3 images"
./bin/fish_cam_rpi --burst 3 --output-dir "${OUTPUT_DIR}"

echo "==> Timelapse: 3 frames every 2 seconds"
./bin/fish_cam_rpi --timelapse 2 --count 3 --output-dir "${OUTPUT_DIR}"

echo "==> Demo capture demo finished. Images in: ${OUTPUT_DIR}"
