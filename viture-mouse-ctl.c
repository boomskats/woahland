#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define SOCKET_PATH_ENV "VITURE_MOUSE_SOCKET"
#define DEFAULT_SOCKET_PATH "/tmp/viture-head-mouse.sock"
#define USER_SOCKET_PATH "/tmp/viture-head-mouse-user.sock"

static void print_usage(const char *prog) {
    printf("Usage: %s COMMAND [ARGS]\n", prog);
    printf("\nCommands:\n");
    printf("  toggle              Toggle head tracking on/off\n");
    printf("  recenter            Reset center position to current orientation\n");
    printf("  pause               Temporarily pause tracking\n");
    printf("  resume              Resume tracking after pause\n");
    printf("  reload              Reload configuration from file\n");
    printf("  status              Show current status\n");
    printf("  sensitivity VALUE   Set sensitivity (e.g., 45)\n");
    printf("  sensitivity +/-VAL  Adjust sensitivity (e.g., +5, -5)\n");
    printf("\nEnvironment:\n");
    printf("  VITURE_MOUSE_SOCKET  Override socket path (default: %s)\n", DEFAULT_SOCKET_PATH);
}

static const char* get_socket_path() {
    const char *path = getenv(SOCKET_PATH_ENV);
    if (path) return path;
    
    // Use different socket paths for root vs user
    return (getuid() == 0) ? DEFAULT_SOCKET_PATH : USER_SOCKET_PATH;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Build command string
    char command[256];
    if (strcmp(argv[1], "sensitivity") == 0 && argc >= 3) {
        snprintf(command, sizeof(command), "sensitivity %s", argv[2]);
    } else if (argc == 2) {
        strncpy(command, argv[1], sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
    } else {
        fprintf(stderr, "Error: Invalid command\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Create socket
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Connect to server
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    const char *socket_path = get_socket_path();
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno == ENOENT || errno == ECONNREFUSED) {
            fprintf(stderr, "Error: viture-head-mouse is not running\n");
        } else {
            perror("Failed to connect to socket");
        }
        close(sock);
        return 1;
    }
    
    // Send command
    if (send(sock, command, strlen(command), 0) < 0) {
        perror("Failed to send command");
        close(sock);
        return 1;
    }
    
    // Read response
    char response[256];
    ssize_t n = recv(sock, response, sizeof(response) - 1, 0);
    if (n > 0) {
        response[n] = '\0';
        printf("%s", response);
        
        // Check if command succeeded
        if (strncmp(response, "ERROR:", 6) == 0) {
            close(sock);
            return 1;
        }
    }
    
    close(sock);
    return 0;
}