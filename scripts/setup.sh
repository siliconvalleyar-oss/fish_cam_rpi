#!/usr/bin/env bash
# =============================================================================
# scripts/setup.sh
#
# One-shot setup for fish_cam_rpi:
#   1. Installs all dependencies (script_tools/install_dependencies.sh).
#   2. Builds the project.
#   3. Optionally runs the demo to validate the pipeline.
#
# Usage:
#   ./scripts/setup.sh [--skip-deps] [--skip-demo]
# =============================================================================

set -euo pipefail

SKIP_DEPS=0
SKIP_DEMO=0

for arg in "$@"; do
  case "${arg}" in
    --skip-deps) SKIP_DEPS=1 ;;
    --skip-demo) SKIP_DEMO=1 ;;
    *)
      echo "Unknown option: ${arg}" >&2
      exit 1
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "${SCRIPT_DIR}")"

cd "${ROOT_DIR}"

if [[ "${SKIP_DEPS}" -eq 0 ]]; then
  echo "==> Installing dependencies..."
  ./script_tools/install_dependencies.sh "${EXTRA_ARGS:-}"
fi

echo "==> Building fish_cam_rpi..."
make

if [[ "${SKIP_DEMO}" -eq 0 ]]; then
  echo "==> Running demo (no camera required)..."
  make demo
  echo "==> Demo images written to ./captures"
fi

echo "==> Setup finished."
echo "    - Binary: ./bin/fish_cam_rpi"
echo "    - Try:    ./bin/fish_cam_rpi --help"
