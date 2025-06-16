#!/bin/bash

# Test script to check if permissions are properly configured

echo "Checking viture-head-mouse permissions..."
echo

# Check if user is in input group
if groups | grep -q input; then
    echo "✓ User is in 'input' group"
else
    echo "✗ User is NOT in 'input' group"
    echo "  Run: sudo usermod -a -G input \$(whoami)"
fi

# Check uinput module
if lsmod | grep -q uinput; then
    echo "✓ uinput module is loaded"
else
    echo "✗ uinput module is NOT loaded"
    echo "  Run: sudo modprobe uinput"
fi

# Check /dev/uinput permissions
if [ -e /dev/uinput ]; then
    if [ -r /dev/uinput ] && [ -w /dev/uinput ]; then
        echo "✓ /dev/uinput is accessible"
    else
        echo "✗ /dev/uinput exists but is not accessible"
        ls -l /dev/uinput
        echo "  You may need to log out and back in, or run the setup script"
    fi
else
    echo "✗ /dev/uinput does not exist"
    echo "  Load the uinput module: sudo modprobe uinput"
fi

echo

# Test socket access by checking if we can create a test socket
TEST_SOCKET="/tmp/viture-test-socket.$$"
if touch "$TEST_SOCKET" 2>/dev/null; then
    echo "✓ Can create socket files in /tmp"
    rm -f "$TEST_SOCKET"
else
    echo "✗ Cannot create socket files in /tmp"
fi

echo

# Summary
if groups | grep -q input && [ -r /dev/uinput ] && [ -w /dev/uinput ]; then
    echo "🎉 All permissions look good! You should be able to run without sudo."
else
    echo "⚠️  Some permissions need fixing. Run ./setup-permissions.sh"
fi