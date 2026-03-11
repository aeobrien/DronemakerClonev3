# Kiosk Mode Setup — Raspberry Pi

## Prerequisites

- Raspberry Pi 5 with Pi OS (64-bit) installed
- SSH access working
- DronemakerClonev3 cloned and building successfully in `~/DronemakerClonev3/`
- JUCE installed at `~/JUCE`
- Scarlett Solo (or other USB audio) connected
- Touchscreen connected and working

## Install the Kiosk Service

SSH into the Pi and run:

```bash
# Make scripts executable
chmod +x ~/DronemakerClonev3/pi-kiosk/kiosk-start.sh
chmod +x ~/DronemakerClonev3/pi-kiosk/update.sh

# Install the systemd service
sudo cp ~/DronemakerClonev3/pi-kiosk/dronemaker.service /etc/systemd/system/
sudo systemctl daemon-reload

# Enable it (starts on boot)
sudo systemctl enable dronemaker

# Disable the desktop environment auto-start (optional but recommended)
sudo systemctl set-default multi-user.target
```

## Reboot into Kiosk Mode

```bash
sudo reboot
```

The Pi will boot, auto-login, and launch DronemakerClone fullscreen.

## Skip Kiosk Mode (Boot to Desktop)

To temporarily boot to the desktop instead of kiosk:

```bash
# Create the skip file (via SSH)
sudo touch /boot/firmware/desktop-mode

# Reboot — will start desktop instead of kiosk
sudo reboot
```

To re-enable kiosk mode:

```bash
sudo rm /boot/firmware/desktop-mode
sudo reboot
```

## Update the Software

From your Mac (or any machine with SSH access):

```bash
ssh pi@pi.local '~/DronemakerClonev3/pi-kiosk/update.sh'
```

Or manually:

```bash
ssh pi@pi.local
sudo systemctl stop dronemaker
cd ~/DronemakerClonev3
git pull
cmake --build build/ -j4
sudo systemctl start dronemaker
```

Or just rebuild and reboot:

```bash
ssh pi@pi.local
cd ~/DronemakerClonev3
git pull
cmake --build build/ -j4
sudo reboot
```

## Useful Commands

```bash
# Check if the service is running
sudo systemctl status dronemaker

# View live logs (stderr output, MIDI messages, etc.)
journalctl -u dronemaker -f

# Stop the app
sudo systemctl stop dronemaker

# Start the app
sudo systemctl start dronemaker

# Restart the app
sudo systemctl restart dronemaker

# Disable kiosk (won't start on boot)
sudo systemctl disable dronemaker

# Re-enable kiosk
sudo systemctl enable dronemaker

# Restore desktop environment as default boot target
sudo systemctl set-default graphical.target
```

## Troubleshooting

### Black screen on boot
- SSH in and check logs: `journalctl -u dronemaker -n 50`
- Check if the binary exists: `ls -la ~/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClonev3`
- Try running manually: `sudo systemctl stop dronemaker && DISPLAY=:0 ~/Dev/.../DronemakerClonev3 --pi-layout`

### No audio
- Check device is connected: `aplay -l`
- Check logs for auto-selection message: `journalctl -u dronemaker | grep "Auto-selected"`
- Try opening Settings in the app (you'll need to temporarily skip kiosk mode)

### No MIDI
- Check device is connected: `amidi -l`
- MIDI devices are auto-scanned every ~2 seconds, so just wait a moment after plugging in
- Check logs: `journalctl -u dronemaker | grep "MIDI"`

### App crashes and restarts in a loop
- The watchdog (`Restart=always`) will keep restarting it
- Stop it: `sudo systemctl stop dronemaker`
- Check the crash logs: `journalctl -u dronemaker -n 100`
- Fix the issue, rebuild, then restart

### Want to go back to normal desktop permanently
```bash
sudo systemctl disable dronemaker
sudo systemctl set-default graphical.target
sudo reboot
```

## Configuration

### Change the username
If your Pi username isn't `aidan`, edit:
- `pi-kiosk/dronemaker.service` — change `User=` and paths
- `pi-kiosk/kiosk-start.sh` — change `APP_PATH`
- `pi-kiosk/update.sh` — change `PROJECT_DIR`

### Change audio buffer size
Edit `MainComponent.cpp`, line with `setup.bufferSize = 2048` — try 1024 for lower latency or 4096 if you get audio glitches.

### Change default MIDI CC assignments
Edit `VirtualEncoderBank.h` — `defaultCCBase`, `defaultNoteBase`, `loopToggleNoteBase`.
