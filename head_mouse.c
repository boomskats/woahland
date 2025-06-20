#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <getopt.h>

#include "viture.h"
#include "mouse_config.h"
#include "socket_server.h"

// Tracking state
typedef struct {
    float last_yaw;
    float last_pitch;
    float last_roll;
    float last_dx;
    float last_dy;
    bool initialized;
    
    // Sub-pixel precision accumulators
    float accum_x;              // Accumulator for sub-pixel X movement
    float accum_y;              // Accumulator for sub-pixel Y movement
    
    // Absolute positioning vars
    float center_yaw;           // Center yaw value for absolute positioning
    float center_pitch;         // Center pitch value for absolute positioning
} MouseState;

// Global variables
static Display *display = NULL;
static MouseConfig config = {
    .sensitivity_yaw = 45.0,    // Higher sensitivity for fixed-cursor feel
    .sensitivity_pitch = 45.0,  // Higher sensitivity for fixed-cursor feel
    .deadzone = 0.0,            // No deadzone for immediate response
    .smoothing = 0.0,           // No smoothing for direct control
    .roll_scroll_threshold = 20.0, // Degrees of roll to trigger scrolling
    .scroll_sensitivity = 0.1,  // Scroll sensitivity
    .invert_x = false,
    .invert_y = true,           // Inverted Y for natural movement
    .invert_scroll = false,
    .yaw_range = 40.0,          // 40 degrees yaw covers screen width
    .pitch_range = 25.0         // 25 degrees pitch covers screen height
};
static MouseState state = {
    .initialized = false
};
static bool enabled = true;
static bool paused = false;
static bool debug_mode = false;
static SocketServer socket_server;

// Convert byte array to float (from SDK example)
static float makeFloat(uint8_t *data)
{
    float value = 0;
    uint8_t tem[4];
    tem[0] = data[3];
    tem[1] = data[2];
    tem[2] = data[1];
    tem[3] = data[0];
    memcpy(&value, tem, 4);
    return value;
}

// IMU data callback from glasses
static void imuCallback(uint8_t *data, uint16_t len, uint32_t ts)
{
    if (!enabled || paused || !display) return;
    
    // Extract Euler angles
    float roll = makeFloat(data);
    float pitch = makeFloat(data + 4);
    float yaw = makeFloat(data + 8);
    
    // Debug output
    if (debug_mode) {
        printf("IMU: roll=%f pitch=%f yaw=%f\n", roll, pitch, yaw);
    }
    
    // Initialize reference position if needed
    if (!state.initialized) {
        state.last_yaw = yaw;
        state.last_pitch = pitch;
        state.last_roll = roll;
        state.center_yaw = yaw;      // Store center position
        state.center_pitch = pitch;  // Store center position
        state.accum_x = 0.0f;
        state.accum_y = 0.0f;
        state.initialized = true;
        return;
    }
    
    // ---- MOUSE MOVEMENT CONTROL ----
    
    // Quaternion data for more accurate orientation (if available)
    float quaternionW = 0.0f, quaternionX = 0.0f, quaternionY = 0.0f, quaternionZ = 0.0f;
    bool have_quaternion = false;
    
    if (len >= 36) {
        quaternionW = makeFloat(data + 20);
        quaternionX = makeFloat(data + 24);
        quaternionY = makeFloat(data + 28);
        quaternionZ = makeFloat(data + 32);
        have_quaternion = true;
    }
    
    // Calculate relative movement
    float delta_yaw = yaw - state.last_yaw;
    float delta_pitch = pitch - state.last_pitch;
    
    // Handle angle wrap-around (yaw goes from -180 to 180)
    if (delta_yaw > 180.0f) delta_yaw -= 360.0f;
    if (delta_yaw < -180.0f) delta_yaw += 360.0f;
    
    // Apply dead zone with smooth transition
    if (fabs(delta_yaw) < config.deadzone) {
        delta_yaw = 0.0f;
    } else {
        // Smooth transition from deadzone
        float sign = delta_yaw > 0 ? 1.0f : -1.0f;
        delta_yaw = sign * (fabs(delta_yaw) - config.deadzone);
    }
    
    if (fabs(delta_pitch) < config.deadzone) {
        delta_pitch = 0.0f;
    } else {
        // Smooth transition from deadzone
        float sign = delta_pitch > 0 ? 1.0f : -1.0f;
        delta_pitch = sign * (fabs(delta_pitch) - config.deadzone);
    }
    
    // Apply sensitivity - more responsive with higher values
    float dx = delta_yaw * config.sensitivity_yaw;
    float dy = delta_pitch * config.sensitivity_pitch;
    
    // Apply inversion if configured
    if (config.invert_x) dx = -dx;
    if (config.invert_y) dy = -dy;
    
    // Apply smoothing
    dx = dx * (1.0f - config.smoothing) + state.last_dx * config.smoothing;
    dy = dy * (1.0f - config.smoothing) + state.last_dy * config.smoothing;
    
    // Add to sub-pixel accumulators
    state.accum_x += dx;
    state.accum_y += dy;
    
    // Extract integer pixel movement
    int move_x = (int)state.accum_x;
    int move_y = (int)state.accum_y;
    
    // Keep sub-pixel remainder
    state.accum_x -= move_x;
    state.accum_y -= move_y;
    
    // Move the mouse cursor if there's movement
    if (move_x != 0 || move_y != 0) {
        XTestFakeRelativeMotionEvent(display, move_x, move_y, CurrentTime);
        XFlush(display);
    }
    
    // ---- SCROLL WHEEL CONTROL ----
    
    // Check if roll exceeds threshold for scrolling
    float abs_roll = fabs(roll);
    if (abs_roll > config.roll_scroll_threshold) {
        // Calculate scroll amount based on how far past threshold
        float scroll_amount = (abs_roll - config.roll_scroll_threshold) * config.scroll_sensitivity;
        
        // Determine scroll direction
        int button;
        if (roll > 0) {
            button = config.invert_scroll ? 5 : 4; // Button 4 is scroll up, 5 is scroll down
        } else {
            button = config.invert_scroll ? 4 : 5;
        }
        
        // Send scroll events
        int scroll_clicks = (int)scroll_amount;
        if (scroll_clicks > 0) {
            for (int i = 0; i < scroll_clicks; i++) {
                XTestFakeButtonEvent(display, button, True, CurrentTime);
                XTestFakeButtonEvent(display, button, False, CurrentTime);
                XFlush(display);
            }
            if (debug_mode) {
                printf("Scrolling: direction=%d, clicks=%d\n", button, scroll_clicks);
            }
        }
    }
    
    // Update state for next iteration
    state.last_yaw = yaw;
    state.last_pitch = pitch;
    state.last_roll = roll;
    state.last_dx = dx;
    state.last_dy = dy;
}

// MCU callback from glasses
static void mcuCallback(uint16_t msgid, uint8_t *data, uint16_t len, uint32_t ts)
{
    if (debug_mode) {
        printf("MCU callback: msgid=%d len=%d\n", msgid, len);
    }
    // Could handle device events here
}

// Toggle head tracking on/off
void toggle_tracking()
{
    enabled = !enabled;
    printf("Head tracking %s\n", enabled ? "enabled" : "disabled");
    
    if (!enabled) {
        // Reset state when disabling
        state.initialized = false;
    }
}

// Pause tracking temporarily
void pause_tracking() {
    paused = true;
    printf("Head tracking paused\n");
}

// Resume tracking
void resume_tracking() {
    paused = false;
    printf("Head tracking resumed\n");
}

// Recenter tracking
void recenter_tracking() {
    state.initialized = false;
    printf("Position recentered. Hold still for a moment.\n");
}

// Reload configuration
void reload_configuration() {
    load_config(&config);
    printf("Configuration reloaded\n");
}

// Get current tracking state
bool get_tracking_enabled() {
    return enabled && !paused;
}

// Get current sensitivity
float get_current_sensitivity() {
    return config.sensitivity_yaw;
}

// Set sensitivity
void set_current_sensitivity(float value) {
    config.sensitivity_yaw = config.sensitivity_pitch = value;
    printf("Sensitivity set to %.1f\n", value);
}

// Adjust sensitivity
void adjust_current_sensitivity(float delta) {
    float new_sens = config.sensitivity_yaw + delta;
    if (new_sens > 0) {
        set_current_sensitivity(new_sens);
    }
}

void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("Options:\n");
    printf("  -d, --debug        Enable debug output\n");
    printf("  -c, --config PATH  Load config from specified file\n");
    printf("  -s, --save-config  Save current config to user config file\n");
    printf("  -h, --help         Show this help message\n");
}

int main(int argc, char *argv[])
{
    // Parse command-line options
    static struct option long_options[] = {
        {"debug", no_argument, 0, 'd'},
        {"config", required_argument, 0, 'c'},
        {"save-config", no_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    char *config_path = NULL;
    bool save_config_flag = false;
    
    int opt;
    while ((opt = getopt_long(argc, argv, "dc:sh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                debug_mode = true;
                printf("Debug mode enabled\n");
                break;
            case 'c':
                config_path = optarg;
                break;
            case 's':
                save_config_flag = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Load configuration
    if (config_path) {
        if (!load_config_file(config_path, &config)) {
            fprintf(stderr, "Failed to load config from: %s\n", config_path);
        }
    } else {
        load_config(&config);
    }
    
    // Save configuration if requested
    if (save_config_flag) {
        save_config(&config);
        return 0;
    }
    // Initialize X11 connection
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Error: Could not open X11 display\n");
        return 1;
    }
    
    // Initialize Viture SDK
    printf("Initializing Viture SDK...\n");
    if (!init(imuCallback, mcuCallback)) {
        fprintf(stderr, "Error: Failed to initialize Viture SDK\n");
        XCloseDisplay(display);
        return 1;
    }
    
    // Enable IMU data
    printf("Enabling IMU data...\n");
    int result = set_imu(true);
    if (result != ERR_SUCCESS) {
        fprintf(stderr, "Error: Failed to enable IMU data (error %d)\n", result);
        deinit();
        XCloseDisplay(display);
        return 1;
    }
    
    // Set IMU frequency to 120Hz for smoother tracking
    set_imu_fq(IMU_FREQUENCE_120);
    
    // Initialize and start socket server
    memset(&socket_server, 0, sizeof(socket_server));
    socket_server.on_toggle = toggle_tracking;
    socket_server.on_recenter = recenter_tracking;
    socket_server.on_pause = pause_tracking;
    socket_server.on_resume = resume_tracking;
    socket_server.on_reload = reload_configuration;
    socket_server.get_enabled = get_tracking_enabled;
    socket_server.get_sensitivity = get_current_sensitivity;
    socket_server.set_sensitivity = set_current_sensitivity;
    socket_server.adjust_sensitivity = adjust_current_sensitivity;
    
    if (!start_socket_server(&socket_server)) {
        fprintf(stderr, "Warning: Failed to start socket server\n");
    }
    
    printf("Head mouse control started. Press Enter to toggle on/off, type 'quit' to exit.\n");
    
    // Main loop - handle user commands
    char input_buffer[256];
    while (1) {
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            // Remove newline if present
            size_t len = strlen(input_buffer);
            if (len > 0 && input_buffer[len-1] == '\n') {
                input_buffer[len-1] = '\0';
            }
            
            if (strlen(input_buffer) == 0) {
                toggle_tracking();
            } else if (strcmp(input_buffer, "quit") == 0) {
                printf("Exiting...\n");
                break;
            } else if (strncmp(input_buffer, "sens ", 5) == 0) {
                float new_sens = atof(input_buffer + 5);
                if (new_sens > 0) {
                    config.sensitivity_yaw = config.sensitivity_pitch = new_sens;
                    printf("Sensitivity set to %.2f\n", new_sens);
                }
            } else if (strncmp(input_buffer, "roll ", 5) == 0) {
                float new_threshold = atof(input_buffer + 5);
                if (new_threshold >= 0) {
                    config.roll_scroll_threshold = new_threshold;
                    printf("Roll scroll threshold set to %.2f degrees\n", new_threshold);
                }
            } else if (strncmp(input_buffer, "scroll ", 7) == 0) {
               float new_sens = atof(input_buffer + 7);
               if (new_sens > 0) {
                   config.scroll_sensitivity = new_sens;
                   printf("Scroll sensitivity set to %.2f\n", new_sens);
               }
           } else if (strncmp(input_buffer, "smooth ", 7) == 0) {
               float new_smooth = atof(input_buffer + 7);
               if (new_smooth >= 0.0f && new_smooth <= 1.0f) {
                   config.smoothing = new_smooth;
                   printf("Smoothing set to %.2f (0.0 = no smoothing, 1.0 = max smoothing)\n", new_smooth);
               }
           } else if (strncmp(input_buffer, "deadzone ", 9) == 0) {
               float new_deadzone = atof(input_buffer + 9);
               if (new_deadzone >= 0.0f) {
                   config.deadzone = new_deadzone;
                   printf("Deadzone set to %.2f degrees\n", new_deadzone);
               }
           } else if (strcmp(input_buffer, "invertx") == 0) {
               config.invert_x = !config.invert_x;
               printf("X-axis %s\n", config.invert_x ? "inverted" : "normal");
           } else if (strcmp(input_buffer, "inverty") == 0) {
               config.invert_y = !config.invert_y;
               printf("Y-axis %s\n", config.invert_y ? "inverted" : "normal");
           } else if (strcmp(input_buffer, "invertscroll") == 0) {
               config.invert_scroll = !config.invert_scroll;
               printf("Scroll direction %s\n", config.invert_scroll ? "inverted" : "normal");
           } else if (strcmp(input_buffer, "recenter") == 0) {
               recenter_tracking();
           } else if (strcmp(input_buffer, "status") == 0) {
               // Print current configuration
               printf("Current configuration:\n");
               printf("  Sensitivity X/Y: %.1f/%.1f\n", config.sensitivity_yaw, config.sensitivity_pitch);
               printf("  Smoothing: %.2f\n", config.smoothing);
               printf("  Deadzone: %.2f degrees\n", config.deadzone);
               printf("  Roll threshold: %.1f degrees\n", config.roll_scroll_threshold);
               printf("  Scroll sensitivity: %.2f\n", config.scroll_sensitivity);
               printf("  X-axis: %s\n", config.invert_x ? "inverted" : "normal");
               printf("  Y-axis: %s\n", config.invert_y ? "inverted" : "normal");
               printf("  Scroll: %s\n", config.invert_scroll ? "inverted" : "normal");
           } else if (strcmp(input_buffer, "help") == 0) {
               printf("Available commands:\n");
               printf("  <Enter>         - Toggle head tracking on/off\n");
               printf("  sens <float>    - Set sensitivity (e.g., sens 9.0)\n");
               printf("  smooth <float>  - Set smoothing factor (0.0-1.0)\n");
               printf("  deadzone <float>- Set movement deadzone in degrees\n");
               printf("  roll <float>    - Set roll threshold for scrolling\n");
               printf("  scroll <float>  - Set scroll sensitivity\n");
               printf("  invertx         - Toggle X-axis inversion\n");
               printf("  inverty         - Toggle Y-axis inversion\n");
               printf("  invertscroll    - Toggle scroll direction inversion\n");
               printf("  recenter        - Reset to current head orientation\n");
               printf("  status          - Display current settings\n");
               printf("  save            - Save current settings to config file\n");
               printf("  reload          - Reload settings from config file\n");
               printf("  quit            - Exit the program\n");
               printf("  debug           - Toggle debug output\n");
               printf("  help            - Show this help\n");
           } else if (strcmp(input_buffer, "debug") == 0) {
               debug_mode = !debug_mode;
               printf("Debug mode %s\n", debug_mode ? "enabled" : "disabled");
           } else if (strcmp(input_buffer, "save") == 0) {
               save_config(&config);
           } else if (strcmp(input_buffer, "reload") == 0) {
               reload_configuration();
           } else {
               printf("Unknown command. Type 'help' for available commands.\n");
           }
        }
    }
    
    // Cleanup
    stop_socket_server(&socket_server);
    set_imu(false);
    deinit();
    XCloseDisplay(display);
    return 0;
}