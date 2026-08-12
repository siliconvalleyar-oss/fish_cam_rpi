#!/usr/bin/env bash
# =============================================================================
# script_tools/kill_fish_cam.sh
#
# Lists every running fish_cam_rpi process and offers to kill them, one by one
# or all at once (e.g. to free /dev/video0 when a capture is stuck).
#
# Usage:
#   ./script_tools/kill_fish_cam.sh           interactive menu
#   ./script_tools/kill_fish_cam.sh --all     kill every fish_cam_rpi process
#   ./script_tools/kill_fish_cam.sh --list    only list the PIDs
#
# Exit codes: 0 = ok, 1 = nothing running / bad usage.
# =============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PROC_PATTERN="/[b]in/fish_cam_rpi"
ALL=0
LIST_ONLY=0

for arg in "$@"; do
  case "${arg}" in
    --all)  ALL=1 ;;
    --list) LIST_ONLY=1 ;;
    *)
      echo "Usage: $0 [--all|--list]"
      echo "  --all    kill every fish_cam_rpi process"
      echo "  --list   only print the PIDs"
      exit 1
      ;;
  esac
done

list_pids() {
  pgrep -f "${PROC_PATTERN}" 2>/dev/null || true
}

pids=( $(list_pids) )
if [[ "${#pids[@]}" -eq 0 ]]; then
  echo "No fish_cam_rpi process is running."
  exit 1
fi

if [[ "${LIST_ONLY}" -eq 1 ]]; then
  printf '%s\n' "${pids[@]}"
  exit 0
fi

echo "Running fish_cam_rpi processes:"
echo "-----------------------------------------------"
echo "  PID  ELAPSED  CMD"
for pid in "${pids[@]}"; do
  ps -o pid=,etime=,stat=,cmd= -p "${pid}" | sed -E 's/^ +/  /'
done
echo "-----------------------------------------------"

kill_one() {
  local pid="$1"
  kill "${pid}" 2>/dev/null && echo "  [KILLED] ${pid} (SIGTERM)" || \
    { kill -9 "${pid}" 2>/dev/null && echo "  [KILLED] ${pid} (SIGKILL)" || \
      echo "  [ERROR] could not kill ${pid}"; }
  sleep 1
  if kill -0 "${pid}" 2>/dev/null; then
    kill -9 "${pid}" 2>/dev/null && echo "  [KILLED] ${pid} (SIGKILL, forced)"
  fi
}

if [[ "${ALL}" -eq 1 ]]; then
  echo "Killing all fish_cam_rpi processes..."
  for pid in "${pids[@]}"; do
    kill_one "${pid}"
  done
  echo "Done. Remaining: $(list_pids | wc -l) process(es)."
  exit 0
fi

# Interactive menu
echo
echo "Choose an option:"
echo "  all   - kill ALL of the processes above"
echo "  <pid> - kill only that PID"
echo "  q     - quit without killing anything"
while true; do
  read -r -p "> " choice || break
  case "${choice}" in
    q|Q|quit|exit) echo "Nothing killed."; exit 0 ;;
    all|ALL)   ALL=1; break ;;
    *)
      if [[ "${choice}" =~ ^[0-9]+$ ]] && kill -0 "${choice}" 2>/dev/null; then
        kill_one "${choice}"
        echo "Remaining: $(list_pids | wc -l) process(es)."
        exit 0
      fi
      echo "Unknown PID or invalid choice. Try again (all / <pid> / q)."
      ;;
  esac
done

echo "Killing all fish_cam_rpi processes..."
for pid in "${pids[@]}"; do
  kill_one "${pid}"
done
echo "Done. Remaining: $(list_pids | wc -l) process(es)."
