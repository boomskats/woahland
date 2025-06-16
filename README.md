# woahland

> *Head tracking-based mouse control for Viture Pro XR glasses on Linux*

Turns your head movements into mouse cursor movements.  Originally built for Wayland, but X11 support turned out to be easy enough to add.

## What is this?

woahland uses the IMU data from Viture glasses to control the mouse cursor. It lets you anchor the mouse cursor in the center of your view while panning the virtual desktop around it with head movements. The initial use case was me wanting to walk around with a Steam Deck in my bag and a split Bluetooth keyboard in my pockets, toggling head tracking on and off for occasional cursor control. 

However, it turned out to also add some stabilising comfort by anchoring a mouse cursor to a fixed place in the center of your view, providing something to focus on when the screens are all over the place. So that's also nice.

Core functionality:
- Head tracking via 120Hz IMU data from Viture glasses (surprisingly great)
- Fixed cursor with head-controlled desktop panning (default)
- Roll-to-scroll by tilting your head (less great, needs work)
- Keybinding integration for window managers via RPC client binary
- Works on both Wayland and X11
- Runtime configuration without restart

## How It Works

Reads IMU data at 120Hz from the glasses. The defaults are tuned for the "fixed cursor" experience - sensitivity of 45 tries to make head movement feel like you're moving the world around a stationary cursor. Y-axis is inverted (looking up moves content down) and rolling your head beyond 20° triggers scrolling.

## Quick Start

### Prerequisites

- Viture Pro XR glasses
- Linux system with X11 or Wayland
- CMake and build tools
- Viture Linux SDK (see build instructions)

#### Fedora/RHEL Package Requirements
```bash
# Basic build tools
sudo dnf install cmake gcc-c++ libX11-devel libXtst-devel

# For Wayland version (uinput support)
sudo dnf install kernel-devel kernel-headers
```

#### Ubuntu/Debian Package Requirements
```bash
# Basic build tools
sudo apt install build-essential cmake libx11-dev libxtst-dev

# For Wayland version (uinput support)
sudo apt install linux-headers-$(uname -r)
```

#### Kernel Module (Wayland version)
The Wayland version requires the `uinput` kernel module:
```bash
# Load module (temporary)
sudo modprobe uinput

# Auto-load on boot
echo 'uinput' | sudo tee /etc/modules-load.d/uinput.conf
```

### Build

1. **Download the Viture Linux SDK** from Viture's official sources
2. **Extract the SDK** and note the location of `libviture_one_sdk.so` and headers
3. **Clone and build woahland:**

```bash
git clone https://github.com/boomskats/woahland
cd woahland

# Copy SDK files (adjust paths as needed)
cp /path/to/viture_sdk/lib/libviture_one_sdk.so libs/
cp /path/to/viture_sdk/include/viture.h include/

mkdir build && cd build
cmake ..

# Build everything
make

# Or build specific versions:
make wayland  # Wayland/uinput version only
make x11      # X11 version only
```

**Note:** The Viture SDK is not included in this repository due to unclear licensing terms. You must download it separately from Viture.

**Build Options:**
- `make` - Build both versions
- `make wayland` - Build Wayland/uinput version only (recommended)
- `make x11` - Build X11 version only

### Setup Permissions (Wayland users)

```bash
# One-time setup to avoid needing sudo
./setup-permissions.sh
# Log out and back in, then verify:
./test-permissions.sh
```

### Run

```bash
# Wayland/uinput version (recommended)
./run_head_mouse_wayland.sh

# X11 version
./run_head_mouse_x11.sh
```

The cursor will appear fixed in the center of your view. Move your head to pan around. It might feel backwards for a while, but should start to feel natural ish pretty quickly.

## Usage

### Basic Controls

Once running, you can control woahland through the console to do the initial finetuning:

- **Enter** - Toggle head tracking on/off
- **`recenter`** - Reset center position to current head orientation  
- **`sens 60`** - Set sensitivity (higher = faster movement)
- **`status`** - Show current configuration
- **`help`** - Prints some stuff
- **`quit`** - Exit

### Keybinding Control

Really you'll want to bind controls to keys in your window manager:

```bash
# Toggle tracking on/off
viture-mouse-ctl toggle

# Recenter position  
viture-mouse-ctl recenter

# Adjust sensitivity on the fly
viture-mouse-ctl sensitivity +10
viture-mouse-ctl sensitivity -5

# Pause/resume (keeps state vs toggle which resets)
viture-mouse-ctl pause
viture-mouse-ctl resume
```

See [example-keybindings.md](example-keybindings.md) for Sway, Niri, and i3 configurations.

### Configuration

woahland looks for config files in this order:
1. `~/.config/viture-head-mouse/config.conf` (user config)
2. `/etc/viture-head-mouse.conf` (system config)  
3. Built-in defaults

Save your current settings:
```bash
# From console
save

# From command line  
viture-mouse-ctl reload
```

Example config file:
```
# Mouse sensitivity (higher = faster movement)
sensitivity_yaw = 45.0
sensitivity_pitch = 45.0

# Movement filtering  
deadzone = 0.0
smoothing = 0.0

# Scroll control
roll_scroll_threshold = 20.0
scroll_sensitivity = 0.1

# Axis inversion
invert_x = false
invert_y = true
invert_scroll = false
```

## Advanced Usage

### Command Line Options

```bash
# Enable debug output
./head_mouse_wayland -d

# Use custom config file
./head_mouse_wayland -c /path/to/config.conf

# Save current config and exit
./head_mouse_wayland -s
```

### Custom Socket Path

```bash
export VITURE_MOUSE_SOCKET="/tmp/my-custom-socket.sock"
./head_mouse_wayland
viture-mouse-ctl toggle  # Will use the same custom socket
```

## Troubleshooting

### "Failed to initialize Viture SDK"
- Check that your Viture glasses are connected and powered on
- Verify the USB connection (magnetic connector can be finicky)
- Try unplugging and reconnecting

### "Could not open X11 display" 
- Make sure you're running in a graphical session
- Check `echo $DISPLAY` shows something like `:0`

### "Error opening /dev/uinput" (Wayland version)
- Run `./test-permissions.sh` to check setup
- If needed, run `./setup-permissions.sh` and log out/in

### "viture-head-mouse is not running" (control client)
- Make sure the main program is running first
- Check that you're using the same user (root vs regular user use different sockets)

### Movement feels "backwards" or uncomfortable
- Try `viture-mouse-ctl recenter` to reset center position while holding your head level and pointing forward
- Adjust sensitivity: start with 30-60 for most people
- Invert the axes if you like traditional mouse-like behavior


## Contributing

Contributions welcome! This started as a Wayland-specific hack for walking around with AR glasses, but I use it a fair bit now.

Potential improvements:
- Auto-reconnection when the magnetic USB connector inevitably gets bumped
- Scrolling sucks, needs work

## License

MIT License for woahland code - see LICENSE file for details.

**Important:** This project depends on the Viture Linux SDK, which must be downloaded separately and may have different licensing terms. The woahland codebase does not modify the Viture SDK, only links against it.

## Acknowledgments

Built using the Viture Linux SDK. Thanks to Viture for making their IMU data accessible and providing solid hardware that actually works on Linux.
