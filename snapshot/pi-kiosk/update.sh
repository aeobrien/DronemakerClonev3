#!/bin/bash
# Update DronemakerClone on the Pi
# Run via SSH: ssh pi@pi.local '~/DronemakerClonev3/pi-kiosk/update.sh'

set -e

PROJECT_DIR="/home/pi/DronemakerClonev3"
SERVICE_NAME="dronemaker"

echo "=== Stopping $SERVICE_NAME ==="
sudo systemctl stop "$SERVICE_NAME" 2>/dev/null || true

echo "=== Pulling latest code ==="
cd "$PROJECT_DIR"
git pull

echo "=== Building ==="
cmake --build build/ -j4

if [ $? -eq 0 ]; then
    echo "=== Build succeeded ==="
    echo "=== Starting $SERVICE_NAME ==="
    sudo systemctl start "$SERVICE_NAME"
    echo "=== Done. App should be running. ==="
else
    echo "!!! BUILD FAILED — not restarting the service !!!"
    echo "Fix the issue, then run: sudo systemctl start $SERVICE_NAME"
    exit 1
fi
