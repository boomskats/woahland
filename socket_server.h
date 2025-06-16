#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <stdbool.h>
#include <pthread.h>

// Socket path
#define SOCKET_PATH_ENV "VITURE_MOUSE_SOCKET"
#define DEFAULT_SOCKET_PATH "/tmp/viture-head-mouse.sock"
#define USER_SOCKET_PATH "/tmp/viture-head-mouse-user.sock"

// Socket server state
typedef struct {
    int socket_fd;
    pthread_t thread;
    bool running;
    
    // Callbacks for commands
    void (*on_toggle)(void);
    void (*on_recenter)(void);
    void (*on_pause)(void);
    void (*on_resume)(void);
    void (*on_reload)(void);
    bool (*get_enabled)(void);
    float (*get_sensitivity)(void);
    void (*set_sensitivity)(float value);
    void (*adjust_sensitivity)(float delta);
} SocketServer;

// Initialize and start the socket server
bool start_socket_server(SocketServer *server);

// Stop the socket server
void stop_socket_server(SocketServer *server);

#endif // SOCKET_SERVER_H