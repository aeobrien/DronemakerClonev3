#!/bin/bash
# Kiosk launcher for DronemakerClone on Raspberry Pi
# Started by systemd dronemaker.service

set -e

APP_PATH="/home/pi/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClonev3"

# Check for desktop-mode skip file
if [ -f /boot/firmware/desktop-mode ] || [ -f /boot/desktop-mode ]; then
    echo "desktop-mode file found — skipping kiosk, starting desktop"
    # Start the desktop environment instead
    exec startx
    exit 0
fi

# Start X server with no window manager, just our app
exec xinit "$APP_PATH" --kiosk --pi-layout -- :0 vt1 \
    -nocursor \
    -nolisten tcp \
    2>&1
