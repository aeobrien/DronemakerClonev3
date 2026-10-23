# Kiosk Mode Setup — Raspberry Pi

## Prerequisites

- Raspberry Pi 5 with Pi OS (64-bit) installed
- SSH access working
- Auto-login enabled on tty1 (default on Pi OS)
- DronemakerClonev3 cloned and building successfully in `~/DronemakerClonev3/`
- JUCE installed at `~/JUCE`
- Scarlett Solo (or other USB audio) connected
- Touchscreen connected and working

## How It Works

The kiosk uses `.bash_profile` to auto-launch the app when the `pi` user logs in on tty1. Pi OS auto-logs in on tty1 by default, so on boot:

1. Pi boots → auto-login on tty1
2. `.bash_profile` runs → detects tty1 with no X display
3. `xinit` starts X server with just the app (no window manager)
4. App runs fullscreen with `--kiosk --pi-layout` flags

A systemd service (`dronemaker.service`) is also included in this directory but is **not used** — it had issues with TTY/SIGHUP on Pi OS. The `.bash_profile` approach is simpler and more reliable.

## Install Kiosk Mode

SSH into the Pi and run:

```bash
# Make the update script executable
chmod +x ~/DronemakerClonev3/pi-kiosk/update.sh

# Disable the desktop environment (boot to console instead)
sudo systemctl set-default multi-user.target

# Add the kiosk launcher to .bash_profile
cat >> ~/.bash_profile << 'KIOSK'

# Kiosk mode - launch app on tty1
if [ "$(tty)" = "/dev/tty1" ] && [ -z "$DISPLAY" ]; then
  if [ ! -f /boot/firmware/desktop-mode ] && [ ! -f /boot/desktop-mode ]; then
    xinit ~/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClone --kiosk --pi-layout -- :0 vt1 -nocursor -nolisten tcp
  fi
fi
KIOSK
```

## Reboot into Kiosk Mode

```bash
sudo reboot
```

The Pi will boot, auto-login, and launch DronemakerClone fullscreen with no title bar.

## Skip Kiosk Mode (Boot to Desktop)

To temporarily boot to the desktop instead of kiosk:

```bash
# Create the skip file (via SSH)
sudo touch /boot/firmware/desktop-mode

# Reboot — will skip kiosk, drop to console
sudo reboot
```

From the console, you can start the desktop with `startx`.

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
cd ~/DronemakerClonev3
git pull
cmake --build build/ -j4
sudo reboot
```

## Safe Shutdown / Reboot

Always shut down the Pi cleanly to avoid SD card corruption:

```bash
# Via SSH from your Mac
ssh pi@pi.local 'sudo shutdown -h now'   # power off
ssh pi@pi.local 'sudo reboot'            # reboot
```

Do **not** just pull the power — this can corrupt the filesystem.

## Useful Commands

```bash
# Kill the running app (via SSH)
pkill -f DronemakerClone

# View system logs
journalctl -xe

# Check if the app process is running
pgrep -a DronemakerClone

# Restore desktop environment as default boot target
sudo systemctl set-default graphical.target

# Check disk usage
df -h
```

## Troubleshooting

### Black screen on boot
- SSH in and check if the binary exists: `ls -la ~/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClone`
- Check `.bash_profile` is correct: `cat ~/.bash_profile`
- Try running manually: `xinit ~/DronemakerClonev3/build/DronemakerClonev3_artefacts/DronemakerClone --kiosk --pi-layout -- :0 vt1`

### App doesn't launch on boot
- Verify auto-login is enabled on tty1: `sudo raspi-config` → System Options → Boot / Auto Login → Console Autologin
- Check that `desktop-mode` skip file doesn't exist: `ls /boot/firmware/desktop-mode`
- Check `.bash_profile` for syntax errors

### No audio
- Check device is connected: `aplay -l`
- The app auto-selects USB audio on startup
- Try running in desktop mode to access the Settings dialog

### No MIDI
- Check device is connected: `amidi -l`
- MIDI devices are auto-scanned every ~2 seconds, so just wait a moment after plugging in

### Settings window trap (known issue)
- If you open the Settings window in kiosk mode, there's no way to close it
- SSH in and kill the app: `pkill -f DronemakerClone`
- The app will restart on next reboot

### Want to go back to normal desktop permanently
```bash
# Remove the kiosk block from .bash_profile (edit manually)
nano ~/.bash_profile
# Delete everything from "# Kiosk mode" to the closing "fi"

# Re-enable desktop
sudo systemctl set-default graphical.target
sudo reboot
```

## Configuration

### Change the username
If your Pi username isn't `pi`, edit the paths in:
- `~/.bash_profile` — the kiosk block
- `pi-kiosk/update.sh` — change `PROJECT_DIR`

### Change audio buffer size
Edit `MainComponent.cpp`, line with `setup.bufferSize = 2048` — try 1024 for lower latency or 4096 if you get audio glitches.

### Change default MIDI CC assignments
Edit `VirtualEncoderBank.h` — `defaultCCBase`, `defaultNoteBase`, `loopToggleNoteBase`.
