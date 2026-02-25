#!/bin/bash

# Build and Setup Script for LoRaWAN Radio Application
# This script compiles the code and optionally installs it as a systemd service

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "[*] Building LoRaWAN Radio Application..."
echo "[*] Build directory: $BUILD_DIR"

# Create build directory if it doesn't exist
if [ ! -d "$BUILD_DIR" ]; then
    echo "[*] Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory and run cmake
cd "$BUILD_DIR"
echo "[*] Running CMake..."
cmake .. || { echo "[ERROR] CMake failed"; exit 1; }

# Build the project
echo "[*] Compiling..."
make -j$(nproc) || { echo "[ERROR] Build failed"; exit 1; }

echo "[SUCCESS] Build completed successfully!"
echo "[*] Binary location: $BUILD_DIR/rpi-sx1262"

# Ask about systemd service installation
echo ""
echo "Would you like to install this as a systemd service to run on boot? (y/n)"
read -r response

if [ "$response" = "y" ] || [ "$response" = "Y" ]; then
    echo "[*] Installing systemd service..."
    
    # Copy service file
    sudo cp "$SCRIPT_DIR/rpi-sx1262.service" /etc/systemd/system/
    
    # Reload systemd daemon
    sudo systemctl daemon-reload
    
    # Enable the service to start on boot
    sudo systemctl enable rpi-sx1262.service
    
    echo "[SUCCESS] Service installed!"
    echo "[*] To start the service now, run: sudo systemctl start rpi-sx1262"
    echo "[*] To check status, run: sudo systemctl status rpi-sx1262"
    echo "[*] To view logs, run: journalctl -u rpi-sx1262 -f"
else
    echo "[*] Service installation skipped"
    echo "[*] To run the application manually: $BUILD_DIR/rpi-sx1262"
fi

echo ""
echo "Would you like to enable a boot hotspot named MFC-1? (y/n)"
read -r hotspot_response

if [ "$hotspot_response" = "y" ] || [ "$hotspot_response" = "Y" ]; then
    echo "[*] Installing hotspot startup service..."

    chmod +x "$SCRIPT_DIR/start_mfc_hotspot.sh"
    sudo cp "$SCRIPT_DIR/mfc-hotspot.service" /etc/systemd/system/
    sudo systemctl daemon-reload
    sudo systemctl enable mfc-hotspot.service

    echo "[SUCCESS] Hotspot service installed!"
    echo "[*] SSID: MFC-1"
    echo "[*] Password: MFC12345"
    echo "[*] To start now, run: sudo systemctl start mfc-hotspot.service"
    echo "[*] To check status, run: sudo systemctl status mfc-hotspot.service"
else
    echo "[*] Hotspot service installation skipped"
fi
