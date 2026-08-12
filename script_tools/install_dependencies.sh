#!/usr/bin/env bash
# =============================================================================
# script_tools/install_dependencies.sh
#
# Installs every dependency needed to build and run fish_cam_rpi on
# Raspberry Pi OS (Debian-based): build tools, OpenCV, codec libraries,
# the raspicam library (built from source) and enables the legacy camera
# stack required by the OV5647 sensor.
#
# Usage:
#   ./script_tools/install_dependencies.sh [--skip-camera-config]
#
# Options:
#   --skip-camera-config   Do not modify /boot config (camera already enabled)
# =============================================================================

set -euo pipefail

RED=$'\033[0;31m'
GREEN=$'\033[0;32m'
YELLOW=$'\033[1;33m'
NC=$'\033[0m'

SKIP_CAMERA_CONFIG=0
if [[ "${1:-}" == "--skip-camera-config" ]]; then
  SKIP_CAMERA_CONFIG=1
fi

log_info()  { printf "%b[INFO]%b %s\n" "${GREEN}" "${NC}" "$*"; }
log_warn()  { printf "%b[WARN]%b %s\n" "${YELLOW}" "${NC}" "$*"; }
log_error() { printf "%b[ERROR]%b %s\n" "${RED}" "${NC}" "$*" >&2; }

# -----------------------------------------------------------------------------
# Sanity checks
# -----------------------------------------------------------------------------
if ! command -v sudo >/dev/null 2>&1; then
  log_error "sudo is required to install packages."
  exit 1
fi

if ! grep -qiE "raspberrypi|raspbian" /etc/os-release 2>/dev/null; then
  log_warn "This script targets Raspberry Pi OS; another distribution was detected."
fi

# -----------------------------------------------------------------------------
# System packages (OpenCV, tools, MMAL headers)
# -----------------------------------------------------------------------------
install_system_packages() {
  log_info "Updating package lists..."
  sudo apt-get update

  log_info "Installing build tools, OpenCV and codec libraries..."
  sudo apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libopencv-dev \
    libjpeg-dev \
    libpng-dev \
    libtiff-dev \
    nlohmann-json3-dev \
    libraspberrypi-dev \
    libraspberrypi-bin \
    libavcodec-dev \
    libavformat-dev \
    libswscale-dev \
    libv4l-dev \
    v4l-utils
}

# -----------------------------------------------------------------------------
# Camera platform detection
# -----------------------------------------------------------------------------
# raspicam needs the legacy MMAL stack (/opt/vc or the MMAL headers/libs),
# which Raspberry Pi OS removed starting with Bookworm (12+). On those
# releases the project builds its OpenCV V4L2 backend instead (libcamera).
has_legacy_camera_stack() {
  [[ -f /opt/vc/lib/libmmal.so ]] || [[ -f /usr/lib/arm-linux-gnueabihf/libmmal.so ]] \
    || [[ -f /usr/include/interface/mmal/mmal.h ]]
}

# -----------------------------------------------------------------------------
# raspicam library (built from source, only when the legacy stack exists)
# -----------------------------------------------------------------------------
build_raspicam() {
  if pkg-config --exists raspicam 2>/dev/null; then
    log_info "raspicam already installed. Skipping build."
    return 0
  fi

  if ! has_legacy_camera_stack; then
    log_warn "Legacy MMAL stack not detected (Raspberry Pi OS Bookworm+)."
    log_warn "raspicam is not available on this release; the project will use"
    log_warn "the OpenCV V4L2 camera backend (libcamera) automatically."
    return 0
  fi

  local workdir="${RASPICAM_DIR:-${HOME}/raspicam}"
  log_info "Building raspicam from https://github.com/cedricve/raspicam.git ..."
  rm -rf "${workdir}"
  GIT_TERMINAL_PROMPT=0 git clone --depth 1 \
      "https://github.com/cedricve/raspicam.git" "${workdir}" || {
    log_error "Failed to clone raspicam."
    return 1
  }

  cmake -S "${workdir}" -B "${workdir}/build" -DCMAKE_BUILD_TYPE=Release \
    && cmake --build "${workdir}/build" -j"$(nproc)" \
    && sudo cmake --install "${workdir}/build" \
    && sudo ldconfig
}

# -----------------------------------------------------------------------------
# Enable the legacy camera stack (only meaningful when the legacy stack exists)
# -----------------------------------------------------------------------------
enable_legacy_camera() {
  if ! has_legacy_camera_stack; then
    log_warn "Legacy stack not present; keeping the libcamera configuration"
    log_warn "(camera_auto_detect) in /boot/firmware/config.txt."
    return 0
  fi

  local config_txt="/boot/config.txt"
  [[ -f /boot/firmware/config.txt ]] && config_txt="/boot/firmware/config.txt"

  log_info "Configuring the legacy camera stack in ${config_txt}"

  local entries=( "camera_auto_detect=0" "start_x=1" "gpu_mem=128" )
  local entry key
  for entry in "${entries[@]}"; do
    key="${entry%%=*}"
    if grep -qE "^[[:space:]]*${key}=" "${config_txt}"; then
      sudo sed -i "s/^[[:space:]]*${key}=.*/${entry}/" "${config_txt}"
      log_info "Updated ${key}= -> ${entry}"
    else
      echo "${entry}" | sudo tee -a "${config_txt}" >/dev/null
      log_info "Appended ${entry}"
    fi
  done

  log_info "Loading the V4L2 compatibility driver..."
  sudo modprobe bcm2835-v4l2 || true
  if ! grep -qxF "bcm2835-v4l2" /etc/modules 2>/dev/null; then
    echo "bcm2835-v4l2" | sudo tee -a /etc/modules >/dev/null
  fi

  log_warn "A REBOOT is required for the camera settings to take effect."
}

# -----------------------------------------------------------------------------
# Verification
# -----------------------------------------------------------------------------
verify() {
  log_info "Verifying installation..."
  for pkg in opencv4 raspicam raspicam_cv; do
    if pkg-config --modversion "${pkg}" >/dev/null 2>&1; then
      log_info "  ${pkg}: $(pkg-config --modversion "${pkg}")"
    else
      log_warn "  ${pkg}: not found by pkg-config"
    fi
  done

  if has_legacy_camera_stack; then
    if command -v vcgencmd >/dev/null 2>&1; then
      log_info "  Camera state: $(vcgencmd get_camera)"
    fi
  else
    log_info "  Camera backend: OpenCV V4L2 (libcamera)."
    log_warn "  If rpicam-hello reports 'No cameras available!', check the"
    log_warn "  CSI ribbon cable (connector labeled CAMERA on Pi 4)."
  fi
}

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
install_system_packages
build_raspicam
[[ "${SKIP_CAMERA_CONFIG}" -eq 0 ]] && enable_legacy_camera
verify

log_info "Done. Reboot with: sudo reboot"
log_info "Then check the camera with: ./script_tools/check_camera.sh"
