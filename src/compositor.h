#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <wayland-server.h>
#include "renderer.h"
#include "desktop.h"
#include "input.h"

#include "xdg-shell.h"

struct dcomp_server {
    struct wl_display *display;
    struct wl_event_loop *loop;

    struct dcomp_renderer *renderer;
    struct dcomp_desktop *desktop;
    struct dcomp_input *input;

    struct wl_list surfaces;   // struct dcomp_surface
    struct wl_list seats;
};

struct dcomp_surface {
    struct wl_resource *resource;
    struct wl_list link;       // server->surfaces

    struct wl_resource *buffer_resource;
    struct wl_listener buffer_release;

    uint32_t width, height;
    int32_t scale;
    bool has_buffer;

    struct dcomp_view *view;
};

struct dcomp_view {
    struct dcomp_surface *surface;
    int32_t x, y;           // position in compositor space
    uint32_t width, height; // logical size
    bool mapped;
    struct wl_list link;    // desktop->views
};

// protocol handlers
void dcomp_wl_compositor_init(struct dcomp_server *server);
void dcomp_xdg_shell_init(struct dcomp_server *server);
void dcomp_wl_seat_init(struct dcomp_server *server);
void dcomp_wl_output_init(struct dcomp_server *server);

// Wayland interface implementations (used by protocol.c)
extern const struct wl_surface_interface surface_impl;
extern const struct wl_compositor_interface compositor_impl;
extern const struct xdg_wm_base_interface xdg_wm_base_impl;
extern const struct wl_seat_interface seat_impl;
extern const struct wl_output_interface output_impl;

// Resource destructors
void destroy_wl_surface(struct wl_resource *resource);
void destroy_wl_compositor(struct wl_resource *resource);

#endif
