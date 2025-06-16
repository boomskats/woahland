#!/bin/bash

# This script demonstrates different sensitivity and smoothness settings
# to help find the optimal configuration for your preferences
# Function to run the head mouse with specific settings
run_with_settings() {
    echo "Running with settings: sensitivity=$1, smoothing=$2, deadzone=$3, roll threshold=$4"
    echo "X-axis inversion: $5, Y-axis inversion: $6"
    echo "Press Enter to toggle tracking, type 'quit' to try next configuration"
    echo "Type 'status' to see all current settings"
    
    # Run the appropriate version based on display server
    if [[ "$XDG_SESSION_TYPE" == "wayland" ]]; then
        # For Wayland
        (cd build && ./run_head_mouse_wayland.sh) << EOF
sens $1
smooth $2
deadzone $3
roll $4
status
$5
$6
status
recenter
quit
EOF
    else
        # For X11
        (cd build && ./run_head_mouse_x11.sh) << EOF
sens $1
smooth $2
deadzone $3
roll $4
status
$5
$6
status
recenter
quit
EOF
    fi
}
}

# Build the application if not already built
mkdir -p build
cd build
cmake ..
make
cd ..

echo "===== Viture Head Mouse Test Settings ====="
echo "This script will let you try different settings."
echo "After each configuration, type 'quit' to move to the next."
echo "========================================"
echo

# Try a few different sensitivity configurations
echo "Configuration 1: High sensitivity, minimal smoothing"
run_with_settings 9.0 0.15 0.1 20.0 "" ""

echo "Configuration 2: Medium sensitivity with X-axis inverted"
run_with_settings 6.0 0.2 0.15 20.0 "invertx" ""

echo "Configuration 3: Lower sensitivity with Y-axis inverted"
run_with_settings 4.0 0.3 0.2 20.0 "" "inverty"

echo "Configuration 4: Very responsive (low smoothing, high sensitivity)"
run_with_settings 12.0 0.05 0.05 15.0 "" ""

echo "Configuration 5: Very smooth (high smoothing, medium sensitivity)"
run_with_settings 6.0 0.6 0.1 20.0 "" ""

echo "All configurations tested. To use your preferred settings, run:"
echo "build/run_head_mouse_x11.sh or build/run_head_mouse_wayland.sh"
echo "Then enter the 'sens', 'smooth', 'deadzone', etc. commands to tune as needed."
echo ""
echo "Available commands:"
echo "  sens <value>     - Set cursor sensitivity"
echo "  smooth <value>   - Set smoothing (0.0-1.0)"
echo "  deadzone <value> - Set minimum movement threshold"
echo "  roll <value>     - Set roll threshold for scrolling"
echo "  invertx          - Toggle X-axis inversion"
echo "  inverty          - Toggle Y-axis inversion"
echo "  invertscroll     - Toggle scroll direction"
echo "  status           - Display current settings"
echo "  recenter         - Reset tracking position"