#define _GNU_SOURCE
#include "compositor.h"
#include "renderer.h"
#include "desktop.h"
#include "input.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <sys/epoll.h>

#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
#include <libinput.h>
#endif

volatile int dcomp_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    dcomp_running = 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct dcomp_server server = {0};
    wl_list_init(&server.surfaces);
    wl_list_init(&server.seats);

    server.display = wl_display_create();
    if (!server.display) {
        fprintf(stderr, "failed to create display\n");
        return 1;
    }

    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        fprintf(stderr, "failed to add socket\n");
        wl_display_destroy(server.display);
        return 1;
    }
    fprintf(stderr, "listening on %s\n", socket);
    setenv("WAYLAND_DISPLAY", socket, 1);

    server.renderer = renderer_create(&server, -1);
    server.desktop = desktop_create(&server);
    server.input = input_create(&server);

    dcomp_wl_compositor_init(&server);
    dcomp_xdg_shell_init(&server);
    dcomp_wl_seat_init(&server);
    dcomp_wl_output_init(&server);

    // Set up event loop with epoll
    struct wl_event_loop *loop = wl_display_get_event_loop(server.display);
    int wayland_fd = wl_event_loop_get_fd(loop);

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev[2];
    ev[0].events = EPOLLIN;
    ev[0].data.fd = wayland_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wayland_fd, &ev[0]);

#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
    int li_fd = -1;
    if (server.input && server.input->li) {
        li_fd = libinput_get_fd(server.input->li);
        ev[1].events = EPOLLIN;
        ev[1].data.fd = li_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, li_fd, &ev[1]);
    }
#endif

    while (dcomp_running) {
        int nfds = epoll_wait(epoll_fd, ev, 2, 50);
        if (nfds < 0 && errno == EINTR) continue;
        if (nfds < 0) break;

        for (int i = 0; i < nfds; i++) {
            if (ev[i].data.fd == wayland_fd) {
                wl_event_loop_dispatch(loop, 0);
            }
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
            if (li_fd >= 0 && ev[i].data.fd == li_fd) {
                if (libinput_next_event(server.input->li) != LIBINPUT_EVENT_NONE) {
                    input_dispatch(server.input);
                }
            }
#endif
        }

        renderer_commit(server.renderer);
        wl_display_flush_clients(server.display);
    }

    fprintf(stderr, "shutting down\n");

    input_destroy(server.input);
    desktop_destroy(server.desktop);
    renderer_destroy(server.renderer);
    wl_display_destroy(server.display);
    close(epoll_fd);

    return 0;
}
