#define _GNU_SOURCE
#include "compositor.h"
#include "renderer.h"
#include "desktop.h"
#include "input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

static int running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct dcomp_server server = {0};
    wl_list_init(&server.surfaces);
    wl_list_init(&server.seats);

    // Create Wayland display
    server.display = wl_display_create();
    if (!server.display) {
        fprintf(stderr, "failed to create display\n");
        return 1;
    }

    // Add socket
    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        fprintf(stderr, "failed to add socket\n");
        wl_display_destroy(server.display);
        return 1;
    }
    fprintf(stderr, "listening on %s\n", socket);
    setenv("WAYLAND_DISPLAY", socket, 1);

    // Create renderer
    server.renderer = renderer_create(&server, -1);

    // Create desktop
    server.desktop = desktop_create(&server);

    // Create input
    server.input = input_create(&server);

    // Init protocols
    dcomp_wl_compositor_init(&server);
    dcomp_xdg_shell_init(&server);
    dcomp_wl_seat_init(&server);
    dcomp_wl_output_init(&server);

    // Main loop
    struct wl_event_loop *loop = wl_display_get_event_loop(server.display);
    while (running) {
        // Dispatch events with 1ms timeout so we can render periodically
        wl_event_loop_dispatch(loop, 1);

        // Render frame
        renderer_commit(server.renderer);

        // Flush client events
        wl_display_flush_clients(server.display);
    }

    fprintf(stderr, "shutting down\n");

    input_destroy(server.input);
    desktop_destroy(server.desktop);
    renderer_destroy(server.renderer);
    wl_display_destroy(server.display);

    return 0;
}
