#include "panel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <vulkan/vulkan.h>

struct panel {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct wl_shm *shm;
    struct wl_seat *seat;
    void *buffer_data;
    int buffer_fd;
};

static struct panel panel;

static void init_wayland(void) {
    panel.display = wl_display_connect(NULL);
    if (!panel.display) {
        fprintf(stderr, "panel: failed to connect\n");
        exit(1);
    }
    panel.compositor = wl_compositor_get(panel.display);
    panel.surface = wl_compositor_create_surface(panel.compositor);
    panel.shm = wl_shm_get(panel.display);
    panel.seat = wl_seat_get(panel.display);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    init_wayland();
    fprintf(stderr, "panel connected\n");
    wl_display_disconnect(panel.display);
    return 0;
}
