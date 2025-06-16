# Example Keybinding Configurations

## Sway

Add these to your Sway config (`~/.config/sway/config`):

```
# Viture head mouse controls
bindsym $mod+F9 exec viture-mouse-ctl toggle
bindsym $mod+F10 exec viture-mouse-ctl recenter
bindsym $mod+F11 exec viture-mouse-ctl pause
bindsym $mod+F12 exec viture-mouse-ctl resume

# Sensitivity adjustment
bindsym $mod+plus exec viture-mouse-ctl sensitivity +5
bindsym $mod+minus exec viture-mouse-ctl sensitivity -5

# Quick status check
bindsym $mod+Shift+F9 exec viture-mouse-ctl status | notify-send -t 2000 "Viture Status" -

# Reload settings
bindsym $mod+Shift+F10 exec viture-mouse-ctl reload
```

## Niri

Add these to your Niri config:

```kdl
binds {
    // Viture head mouse controls
    "Mod+F9" { spawn "viture-mouse-ctl" "toggle"; }
    "Mod+F10" { spawn "viture-mouse-ctl" "recenter"; }
    "Mod+F11" { spawn "viture-mouse-ctl" "pause"; }
    "Mod+F12" { spawn "viture-mouse-ctl" "resume"; }
    
    // Sensitivity adjustment
    "Mod+Plus" { spawn "viture-mouse-ctl" "sensitivity" "+5"; }
    "Mod+Minus" { spawn "viture-mouse-ctl" "sensitivity" "-5"; }
    
    // Reload settings
    "Mod+Shift+F10" { spawn "viture-mouse-ctl" "reload"; }
}
```

## i3/i3-gaps

Add these to your i3 config (`~/.config/i3/config`):

```
# Viture head mouse controls
bindsym $mod+F9 exec viture-mouse-ctl toggle
bindsym $mod+F10 exec viture-mouse-ctl recenter
bindsym $mod+F11 exec viture-mouse-ctl pause
bindsym $mod+F12 exec viture-mouse-ctl resume

# Sensitivity adjustment
bindsym $mod+plus exec viture-mouse-ctl sensitivity +5
bindsym $mod+minus exec viture-mouse-ctl sensitivity -5

# Reload settings
bindsym $mod+Shift+F10 exec viture-mouse-ctl reload
```

## Command Line Usage

```bash
# Toggle tracking on/off
viture-mouse-ctl toggle

# Recenter to current position
viture-mouse-ctl recenter

# Check status
viture-mouse-ctl status

# Adjust sensitivity
viture-mouse-ctl sensitivity 60
viture-mouse-ctl sensitivity +10
viture-mouse-ctl sensitivity -5

# Pause/resume (keeps state vs toggle which resets)
viture-mouse-ctl pause
viture-mouse-ctl resume

# Reload configuration from file
viture-mouse-ctl reload
```

## Custom Socket Path

If you want to use a custom socket path:

```bash
export VITURE_MOUSE_SOCKET="/tmp/my-custom-socket.sock"
./head_mouse_wayland  # Uses custom socket path
viture-mouse-ctl toggle  # Also uses custom socket path
```