#!/usr/bin/env bash
# Stage and zip a release archive for end users.
#
# Output: dist/SkillXPNotify-<version>.zip with the layout a user can
# drop straight into their Skyrim install (paths preserved relative to
# Data/):
#
#   Data/SKSE/Plugins/SkillXPNotify.dll
#   Data/SKSE/Plugins/SkillXPNotify.ini.example
#   README.md
#   LICENSE
#
# Run after a Release build of the SkillXPNotify target. Pulls the
# version from the project()'s VERSION line in CMakeLists.txt.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

DLL_PATH="build/SkillXPNotify.dll"
[[ -f "$DLL_PATH" ]] || {
    echo "error: $DLL_PATH not found. Run a Release build first." >&2
    exit 1
}

VERSION="$(grep -oE 'VERSION[[:space:]]+[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt \
           | head -1 \
           | awk '{print $2}')"
[[ -n "$VERSION" ]] || { echo "error: could not parse version from CMakeLists.txt" >&2; exit 1; }

STAGE="dist/stage-${VERSION}"
OUT_ZIP="dist/SkillXPNotify-${VERSION}.zip"

rm -rf "$STAGE" "$OUT_ZIP"
mkdir -p "$STAGE/Data/SKSE/Plugins"

cp "$DLL_PATH"                 "$STAGE/Data/SKSE/Plugins/SkillXPNotify.dll"
cp "SkillXPNotify.ini.example" "$STAGE/Data/SKSE/Plugins/SkillXPNotify.ini.example"
cp README.md                   "$STAGE/README.md"
cp CHANGELOG.md                "$STAGE/CHANGELOG.md"
cp LICENSE                     "$STAGE/LICENSE"

(cd "$STAGE" && zip -qr "../$(basename "$OUT_ZIP")" .)

echo
echo "=== built ${OUT_ZIP} ==="
unzip -l "$OUT_ZIP"
echo
echo "size: $(du -h "$OUT_ZIP" | awk '{print $1}')"
