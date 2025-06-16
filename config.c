#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>

#include "mouse_config.h"

// Get the user config file path
char* get_user_config_path(void) {
    static char path[1024];
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) return NULL;
    
    snprintf(path, sizeof(path), "%s/%s/%s", home, USER_CONFIG_DIR, USER_CONFIG_FILE);
    return path;
}

// Create config directory if it doesn't exist
static bool ensure_config_dir(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }
    if (!home) return false;
    
    char dir_path[1024];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", home, USER_CONFIG_DIR);
    
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
            fprintf(stderr, "Failed to create config directory: %s\n", strerror(errno));
            return false;
        }
    }
    return true;
}

// Parse a config line
static bool parse_config_line(const char *line, MouseConfig *config) {
    char key[64], value[64];
    if (sscanf(line, "%63s = %63s", key, value) != 2) {
        return false;
    }
    
    if (strcmp(key, "sensitivity_yaw") == 0) {
        config->sensitivity_yaw = atof(value);
    } else if (strcmp(key, "sensitivity_pitch") == 0) {
        config->sensitivity_pitch = atof(value);
    } else if (strcmp(key, "deadzone") == 0) {
        config->deadzone = atof(value);
    } else if (strcmp(key, "smoothing") == 0) {
        config->smoothing = atof(value);
    } else if (strcmp(key, "roll_scroll_threshold") == 0) {
        config->roll_scroll_threshold = atof(value);
    } else if (strcmp(key, "scroll_sensitivity") == 0) {
        config->scroll_sensitivity = atof(value);
    } else if (strcmp(key, "invert_x") == 0) {
        config->invert_x = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "invert_y") == 0) {
        config->invert_y = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "invert_scroll") == 0) {
        config->invert_scroll = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
    } else if (strcmp(key, "yaw_range") == 0) {
        config->yaw_range = atof(value);
    } else if (strcmp(key, "pitch_range") == 0) {
        config->pitch_range = atof(value);
    }
    
    return true;
}

// Load config from file
bool load_config_file(const char *path, MouseConfig *config) {
    FILE *file = fopen(path, "r");
    if (!file) {
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        parse_config_line(line, config);
    }
    
    fclose(file);
    return true;
}

// Save config to file
bool save_config_file(const char *path, const MouseConfig *config) {
    if (!ensure_config_dir()) {
        return false;
    }
    
    FILE *file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open config file for writing: %s\n", strerror(errno));
        return false;
    }
    
    fprintf(file, "# Viture Head Mouse Configuration\n");
    fprintf(file, "# Generated automatically - feel free to edit\n\n");
    
    fprintf(file, "# Mouse sensitivity (higher = faster movement)\n");
    fprintf(file, "sensitivity_yaw = %.1f\n", config->sensitivity_yaw);
    fprintf(file, "sensitivity_pitch = %.1f\n\n", config->sensitivity_pitch);
    
    fprintf(file, "# Movement filtering\n");
    fprintf(file, "deadzone = %.2f\n", config->deadzone);
    fprintf(file, "smoothing = %.2f\n\n", config->smoothing);
    
    fprintf(file, "# Scroll control\n");
    fprintf(file, "roll_scroll_threshold = %.1f\n", config->roll_scroll_threshold);
    fprintf(file, "scroll_sensitivity = %.2f\n\n", config->scroll_sensitivity);
    
    fprintf(file, "# Axis inversion\n");
    fprintf(file, "invert_x = %s\n", config->invert_x ? "true" : "false");
    fprintf(file, "invert_y = %s\n", config->invert_y ? "true" : "false");
    fprintf(file, "invert_scroll = %s\n\n", config->invert_scroll ? "true" : "false");
    
    fprintf(file, "# Screen mapping ranges (degrees)\n");
    fprintf(file, "yaw_range = %.1f\n", config->yaw_range);
    fprintf(file, "pitch_range = %.1f\n", config->pitch_range);
    
    fclose(file);
    return true;
}

// Load config with fallback order: user -> system -> defaults
void load_config(MouseConfig *config) {
    char *user_path = get_user_config_path();
    
    // Try user config first
    if (user_path && load_config_file(user_path, config)) {
        printf("Loaded config from: %s\n", user_path);
        return;
    }
    
    // Try system config
    if (load_config_file(SYSTEM_CONFIG_PATH, config)) {
        printf("Loaded config from: %s\n", SYSTEM_CONFIG_PATH);
        return;
    }
    
    // Defaults are already set in the MouseConfig initialization
    printf("Using default configuration\n");
}

// Save current config to user file
void save_config(const MouseConfig *config) {
    char *user_path = get_user_config_path();
    if (user_path && save_config_file(user_path, config)) {
        printf("Saved config to: %s\n", user_path);
    } else {
        fprintf(stderr, "Failed to save config\n");
    }
}