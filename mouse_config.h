#ifndef MOUSE_CONFIG_H
#define MOUSE_CONFIG_H

#include <stdbool.h>

// Configuration structure
typedef struct {
    float sensitivity_yaw;      // Sensitivity for horizontal movement
    float sensitivity_pitch;    // Sensitivity for vertical movement
    float deadzone;             // Minimum movement to register (in degrees)
    float smoothing;            // Smoothing factor (0.0-1.0)
    float roll_scroll_threshold; // Roll angle at which to trigger scrolling
    float scroll_sensitivity;   // Sensitivity for scroll wheel
    bool invert_x;              // Invert horizontal movement
    bool invert_y;              // Invert vertical movement
    bool invert_scroll;         // Invert scroll direction
    
    // Screen mapping ranges (in degrees)
    float yaw_range;            // Total yaw range to map to screen width
    float pitch_range;          // Total pitch range to map to screen height
} MouseConfig;

// Default config file locations
#define SYSTEM_CONFIG_PATH "/etc/viture-head-mouse.conf"
#define USER_CONFIG_DIR ".config/viture-head-mouse"
#define USER_CONFIG_FILE "config.conf"

// Config file operations
bool load_config_file(const char *path, MouseConfig *config);
bool save_config_file(const char *path, const MouseConfig *config);
char* get_user_config_path(void);
void load_config(MouseConfig *config);
void save_config(const MouseConfig *config);

#endif // MOUSE_CONFIG_H