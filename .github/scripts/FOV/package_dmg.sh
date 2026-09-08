#!/bin/bash
set -e

SOURCE_DIR="${1:-build_macos}"
OUTPUT_DMG="${2:-FOV-macos-arm64.dmg}"
VOL_NAME="OBS-FOV"

echo "=== Creating DMG from ${SOURCE_DIR} ==="

if [ ! -d "${SOURCE_DIR}" ]; then
    echo "ERROR: Directory '${SOURCE_DIR}' does not exist!"
    exit 1
fi

STAGING_DIR="dmg_staging"
rm -rf "${STAGING_DIR}" "${OUTPUT_DMG}"
mkdir -p "${STAGING_DIR}"

# Search for OBS.app dynamically within the build output
APP_PATH=$(find "${SOURCE_DIR}" -name "OBS.app" -type d | head -n 1)

if [ -n "${APP_PATH}" ]; then
    echo "Found macOS bundle: ${APP_PATH}"
    cp -R "${APP_PATH}" "${STAGING_DIR}/"
elif [ -d "${SOURCE_DIR}/rundir" ]; then
    echo "Found rundir: ${SOURCE_DIR}/rundir"
    cp -R "${SOURCE_DIR}/rundir"/* "${STAGING_DIR}/"
else
    echo "Copying direct contents from ${SOURCE_DIR}..."
    cp -R "${SOURCE_DIR}"/* "${STAGING_DIR}/"
fi

# Create drag-and-drop shortcut link to /Applications
ln -s /Applications "${STAGING_DIR}/Applications"

# Generate compressed read-only DMG using native hdiutil
hdiutil create \
  -volname "${VOL_NAME}" \
  -srcfolder "${STAGING_DIR}" \
  -ov \
  -format UDZO \
  "${OUTPUT_DMG}"

rm -rf "${STAGING_DIR}"
echo "DMG created successfully: ${OUTPUT_DMG}"
