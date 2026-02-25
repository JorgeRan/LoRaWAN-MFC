# LoRaWAN Radio Application - Switch Control Setup Guide

## Overview
This application controls a LoRaWAN radio (SX1262) using a physical switch on your Raspberry Pi. Press the switch to start, press again to stop.

## Hardware Setup
1. **Switch Pin**: GPIO pin 0 (as defined in `main.cpp`)
   - Connect one end of your switch to GPIO pin 0
   - Connect the other end to GND (ground)
   - The switch should be a normally-open switch (not pressed = LOW, pressed = HIGH)

2. **Radio Module**: SX1262 connected via SPI

## Software Setup

### Step 1: Build the Application
The easiest way to compile and install is to use the build script:

```bash
cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy
./build_and_install.sh
```

This script will:
- Create a build directory
- Run CMake
- Compile the code with optimal settings
- Ask if you want to install it as a systemd service

### Step 2: Install as a Systemd Service (Optional but Recommended)

If you chose "yes" during the build script, the service is already installed. Otherwise, you can install it manually:

```bash
sudo cp /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/rpi-sx1262.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable rpi-sx1262.service
```

### Step 3: Running the Application

#### Option A: Manual Run
```bash
cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/build
./rpi-sx1262
```

#### Option B: As a Systemd Service (runs on boot)
Start the service:
```bash
sudo systemctl start rpi-sx1262
```

Check status:
```bash
sudo systemctl status rpi-sx1262
```

View live logs:
```bash
journalctl -u rpi-sx1262 -f
```

Stop the service:
```bash
sudo systemctl stop rpi-sx1262
```

Disable from auto-start:
```bash
sudo systemctl disable rpi-sx1262.service
```

### Step 4: Enable Hotspot on Boot (SSID: MFC-1)

If you chose "yes" in `build_and_install.sh`, the hotspot service is already installed.

Manual install:
```bash
cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy
chmod +x start_mfc_hotspot.sh
sudo cp mfc-hotspot.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable mfc-hotspot.service
```

Start it now without rebooting:
```bash
sudo systemctl start mfc-hotspot.service
```

Check status:
```bash
sudo systemctl status mfc-hotspot.service
```

View logs:
```bash
journalctl -u mfc-hotspot.service -f
```

Notes:
- Default hotspot SSID is `MFC-1`
- Default WPA password is `MFC12345`
- Interface defaults to `wlan0`
- Script uses NetworkManager (`nmcli`) and shared IPv4 mode
- To configure a WPA password, set env var `MFC_HOTSPOT_PASSWORD` in the service before starting

## Operation

1. **Power on** the Raspberry Pi with the battery connected
2. **Press the switch** to start the application
3. The program will initialize the radio and begin sending uplink/downlink messages
4. **Press the switch again** to stop the program gracefully

## Features

- ✅ Switch-controlled startup (prevents accidental startup)
- ✅ Press switch again to stop (clean shutdown)
- ✅ Runs on boot automatically (if systemd service is enabled)
- ✅ No need to plug in a computer after initial setup
- ✅ Automatic logging to systemd journal
- ✅ Button debouncing (50ms) for reliable operation

## Troubleshooting

### Service won't start
Check the logs:
```bash
journalctl -u rpi-sx1262 -n 50
```

### Button not responding
- Verify GPIO pin connection
- Check if button voltage is reaching the Raspberry Pi
- Test with: `gpio -g read 17` (for GPIO pin 17, adjust as needed)

### Service starts but program crashes
Run manually to see detailed error messages:
```bash
cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/build
./rpi-sx1262
```

### Recompile after code changes
```bash
cd /home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy
./build_and_install.sh
```

## Auto-Start on Boot

Once the systemd service is installed, your application will:
1. Automatically start when the Raspberry Pi boots
2. Wait for the switch to be pressed
3. Begin LoRaWAN operations
4. Stop gracefully when the switch is pressed again
5. Automatically restart if it crashes (after 10 seconds)

To view boot startup logs:
```bash
journalctl -u rpi-sx1262 --since "10 minutes ago"
```

## File Locations
- **Application**: `/home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/build/rpi-sx1262`
- **Build script**: `/home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/build_and_install.sh`
- **Service file**: `/etc/systemd/system/rpi-sx1262.service`
- **Source code**: `/home/pi/Documents/Radiolib/examples/NonArduino/Raspberry_copy/main.cpp`
