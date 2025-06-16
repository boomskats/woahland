#!/bin/bash

# Setup script to configure permissions for viture-head-mouse
# This allows running without sudo

set -e

echo "Setting up viture-head-mouse permissions..."

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    echo "Error: Do not run this script as root. Run as your regular user."
    exit 1
fi

# Get current user
CURRENT_USER=$(whoami)

echo "Current user: $CURRENT_USER"

# Add user to input group
echo "Adding $CURRENT_USER to 'input' group..."
sudo usermod -a -G input "$CURRENT_USER"

# Install udev rule
echo "Installing udev rule for /dev/uinput access..."
sudo cp 99-uinput-permissions.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
sudo udevadm trigger

# Check if input group exists and has uinput access
echo "Checking current permissions..."
ls -l /dev/uinput 2>/dev/null || echo "Warning: /dev/uinput not found (may need to load uinput module)"

echo ""
echo "Setup complete! Please log out and log back in for group changes to take effect."
echo ""
echo "After logging back in, you should be able to run:"
echo "  ./head_mouse_wayland"
echo "  viture-mouse-ctl toggle"
echo ""
echo "If /dev/uinput still doesn't exist, run:"
echo "  sudo modprobe uinput"
echo "  echo 'uinput' | sudo tee /etc/modules-load.d/uinput.conf"