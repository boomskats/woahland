#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#include "socket_server.h"

// Get socket path from environment or use default
static const char* get_socket_path() {
    const char *path = getenv(SOCKET_PATH_ENV);
    return path ? path : DEFAULT_SOCKET_PATH;
}

// Handle a client command
static void handle_command(SocketServer *server, int client_fd, const char *cmd) {
    char response[256];
    
    if (strcmp(cmd, "toggle") == 0) {
        if (server->on_toggle) {
            server->on_toggle();
        }
        bool enabled = server->get_enabled ? server->get_enabled() : false;
        snprintf(response, sizeof(response), "OK: tracking %s\n", enabled ? "enabled" : "disabled");
        
    } else if (strcmp(cmd, "recenter") == 0) {
        if (server->on_recenter) {
            server->on_recenter();
        }
        snprintf(response, sizeof(response), "OK: recentered\n");
        
    } else if (strcmp(cmd, "pause") == 0) {
        if (server->on_pause) {
            server->on_pause();
        }
        snprintf(response, sizeof(response), "OK: paused\n");
        
    } else if (strcmp(cmd, "resume") == 0) {
        if (server->on_resume) {
            server->on_resume();
        }
        snprintf(response, sizeof(response), "OK: resumed\n");
        
    } else if (strcmp(cmd, "reload") == 0) {
        if (server->on_reload) {
            server->on_reload();
        }
        snprintf(response, sizeof(response), "OK: configuration reloaded\n");
        
    } else if (strcmp(cmd, "status") == 0) {
        bool enabled = server->get_enabled ? server->get_enabled() : false;
        float sensitivity = server->get_sensitivity ? server->get_sensitivity() : 0.0f;
        snprintf(response, sizeof(response), "OK: enabled=%s sensitivity=%.1f\n", 
                 enabled ? "true" : "false", sensitivity);
                 
    } else if (strncmp(cmd, "sensitivity ", 12) == 0) {
        const char *arg = cmd + 12;
        if (arg[0] == '+' || arg[0] == '-') {
            // Relative adjustment
            float delta = atof(arg);
            if (server->adjust_sensitivity) {
                server->adjust_sensitivity(delta);
            }
            float new_sens = server->get_sensitivity ? server->get_sensitivity() : 0.0f;
            snprintf(response, sizeof(response), "OK: sensitivity adjusted to %.1f\n", new_sens);
        } else {
            // Absolute value
            float value = atof(arg);
            if (value > 0 && server->set_sensitivity) {
                server->set_sensitivity(value);
                snprintf(response, sizeof(response), "OK: sensitivity set to %.1f\n", value);
            } else {
                snprintf(response, sizeof(response), "ERROR: invalid sensitivity value\n");
            }
        }
        
    } else {
        snprintf(response, sizeof(response), "ERROR: unknown command '%s'\n", cmd);
    }
    
    // Send response
    send(client_fd, response, strlen(response), 0);
}

// Socket server thread
static void* socket_server_thread(void *arg) {
    SocketServer *server = (SocketServer *)arg;
    struct sockaddr_un addr;
    
    // Block SIGPIPE to prevent crashes on broken connections
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    
    // Create socket
    server->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server->socket_fd < 0) {
        perror("Failed to create socket");
        return NULL;
    }
    
    // Make socket non-blocking
    int flags = fcntl(server->socket_fd, F_GETFL, 0);
    fcntl(server->socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Remove existing socket file
    const char *socket_path = get_socket_path();
    unlink(socket_path);
    
    // Bind socket
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    if (bind(server->socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Failed to bind socket");
        close(server->socket_fd);
        return NULL;
    }
    
    // Listen for connections
    if (listen(server->socket_fd, 5) < 0) {
        perror("Failed to listen on socket");
        close(server->socket_fd);
        return NULL;
    }
    
    printf("Socket server listening on %s\n", socket_path);
    
    // Main server loop
    while (server->running) {
        fd_set readfds;
        struct timeval timeout;
        
        FD_ZERO(&readfds);
        FD_SET(server->socket_fd, &readfds);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms timeout
        
        int result = select(server->socket_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (result > 0 && FD_ISSET(server->socket_fd, &readfds)) {
            // Accept new connection
            int client_fd = accept(server->socket_fd, NULL, NULL);
            if (client_fd >= 0) {
                // Read command
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                
                ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                if (n > 0) {
                    // Remove trailing newline
                    if (buffer[n-1] == '\n') buffer[n-1] = '\0';
                    
                    // Handle command
                    handle_command(server, client_fd, buffer);
                }
                
                close(client_fd);
            }
        }
    }
    
    // Cleanup
    close(server->socket_fd);
    unlink(socket_path);
    
    return NULL;
}

// Start the socket server
bool start_socket_server(SocketServer *server) {
    server->running = true;
    
    if (pthread_create(&server->thread, NULL, socket_server_thread, server) != 0) {
        perror("Failed to create socket server thread");
        return false;
    }
    
    return true;
}

// Stop the socket server
void stop_socket_server(SocketServer *server) {
    server->running = false;
    pthread_join(server->thread, NULL);
}