#!/bin/bash

set -euo pipefail

SSID="${MFC_HOTSPOT_SSID:-MFC-1}"
IFACE="${MFC_HOTSPOT_IFACE:-wlan0}"
CON_NAME="${MFC_HOTSPOT_CONN_NAME:-mfc-hotspot}"
PASSWORD="${MFC_HOTSPOT_PASSWORD:-}"

if ! command -v nmcli >/dev/null 2>&1; then
    echo "[ERROR] nmcli not found. Install and enable NetworkManager first."
    exit 1
fi

if ! systemctl is-active --quiet NetworkManager; then
    echo "[ERROR] NetworkManager is not running."
    exit 1
fi

if nmcli -t -f NAME connection show | grep -Fxq "$CON_NAME"; then
    nmcli connection delete "$CON_NAME" >/dev/null 2>&1 || true
fi

nmcli connection add type wifi ifname "$IFACE" con-name "$CON_NAME" ssid "$SSID" autoconnect yes

nmcli connection modify "$CON_NAME" \
    connection.interface-name "$IFACE" \
    802-11-wireless.mode ap \
    802-11-wireless.band bg \
    802-11-wireless.ssid "$SSID" \
    ipv4.method shared \
    ipv6.method ignore

if [ -n "$PASSWORD" ] && [ "${#PASSWORD}" -ge 8 ]; then
    nmcli connection modify "$CON_NAME" wifi-sec.key-mgmt wpa-psk wifi-sec.psk "$PASSWORD"
else
    nmcli connection modify "$CON_NAME" wifi-sec.key-mgmt none
fi

nmcli connection up "$CON_NAME"
echo "[OK] Hotspot active: $SSID ($CON_NAME on $IFACE)"
