#!/usr/bin/env bash
# =============================================================================
# scripts/release.sh
#
# Crea una release de fish_cam_rpi:
#   - incrementa la version en 0.0.1 (patch)
#   - actualiza VERSION y docs/CHANGELOG.md
#   - crea el commit y el tag anotado vX.Y.Z
#   - (opcional) hace push de main y del tag
#
# Politica de versionado: cada release incrementa el patch en 0.0.1
#     v0.1.0 -> v0.1.1 -> v0.1.2 -> ...
#
# Usage:
#   ./scripts/release.sh [--push]
# =============================================================================

set -euo pipefail

PUSH=0
if [[ "${1:-}" == "--push" ]]; then
  PUSH=1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${ROOT_DIR}"

VERSION_FILE="VERSION"
CHANGELOG="docs/CHANGELOG.md"

if [[ ! -f "${VERSION_FILE}" ]]; then
  echo "ERROR: missing ${VERSION_FILE}" >&2
  exit 1
fi

# --- Incremento de version (0.0.1) -----------------------------------------
current="$(cat "${VERSION_FILE}")"
IFS='.' read -r major minor patch <<< "${current}"
next="${major}.${minor}.$((patch + 1))"
tag="v${next}"

echo "==> Release: ${current} -> ${next} (tag ${tag})"

# --- Actualizar archivos de version ----------------------------------------
echo "${next}" > "${VERSION_FILE}"
echo "    VERSION actualizado a ${next}"

if [[ -f "${CHANGELOG}" ]]; then
  tmp="$(mktemp)"
  date_today="$(date +%F)"
  awk -v v="${next}" -v d="${date_today}" '
    /^## \[No publicado\]/ {
      print "## [" v "] - " d
      print ""
      print "### Añadido"
      print "- ..."
      print ""
    }
    { print }
  ' "${CHANGELOG}" > "${tmp}" && mv "${tmp}" "${CHANGELOG}"
  echo "    CHANGELOG actualizado"
fi

# --- Commit y tag ------------------------------------------------------------
git add "${VERSION_FILE}" "${CHANGELOG}"
git commit -m "Release ${next}: version bump ${current} -> ${next}"
git tag -a "${tag}" -m "fish_cam_rpi ${next}"

if [[ "${PUSH}" -eq 1 ]]; then
  git push origin main
  git push origin "${tag}"
  echo "==> Publicado: main + ${tag}"
else
  echo "==> Listo. Commit y tag ${tag} creados localmente."
  echo "    Revisa docs/CHANGELOG.md y publica con: ./scripts/release.sh --push"
fi
