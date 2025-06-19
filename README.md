# woahland

<video src="https://github.com/user-attachments/assets/848b74e2-7ad1-4d29-99fe-833f2fe63a0b" autoplay></video>

## What 

`woahland` is a mouse control driver for Viture Pro XR glasses on Linux. It uses the IMU data from Viture glasses to let you control the mouse cursor by moving your head around. 

## Why 

I wanted to try walking around with a Steam Deck in my bag and a split Bluetooth keyboard in my pockets, with the ability to toggle head tracking on and off for occasional cursor control. The IMU unit in the Vitures turned out to be surprisingly great for this, with high accuracy and very low latency, so here we are.

## Features

By default `woahland` uses a "fixed cursor" experience. It lets you anchor the mouse cursor in the center of your view, so it feels like you're moving your desktop around a stationary cursor. This is surprisingly useful for making the whole high-resolution functional virtual desktop AR experience feel a bit more stable. 

You can change all of that by inverting the axes, adjusting the sensitivity, tweaking the deadzone, etc. if you want to.

Core functionality:

- Head tracking via 120Hz IMU data from Viture glasses (surprisingly great)
- Roll-to-scroll by tilting your head (needs work, ramps too quickly)
- Keybinding integration for window managers via RPC client binary
- Works on both Wayland and X11
- Runtime configuration without restart

## Quickstart

You should obviously take a look at what `quick-start.sh` does before you run it, but this should work: 

```bash
  git clone https://github.com/yourusername/woahland.git
  cd woahland
  ./quick-start.sh
  ./build/run_head_mouse_wayland.sh  # Run directly from repo root
```

Then follow the instructions in the console / integrate the viture-mouse-ctl with your keybindings etc.

## Manual build / dev setup

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

This is kinda what the quick-start.sh does already:

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

## License

MIT License for woahland code - see LICENSE file for details.

**Important:** This project depends on the Viture Linux SDK, which must be downloaded separately and may have different licensing terms. The woahland codebase does not modify the Viture SDK, only links against it.

## Acknowledgments

Built using the Viture Linux SDK. Thanks to Viture for making their IMU data accessible and providing solid hardware that actually works on Linux.
