#include "compositor.h"
#include "protocol.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>



static void surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client;
    dcomp_surface_destroy(resource);
}

static void surface_attach(struct wl_client *client, struct wl_resource *resource,
                           struct wl_resource *buffer, int32_t x, int32_t y) {
    dcomp_surface_attach(client, resource, buffer, x, y);
}

static void surface_damage(struct wl_client *client, struct wl_resource *resource,
                           int32_t x, int32_t y, int32_t w, int32_t h) {
    dcomp_surface_damage(client, resource, x, y, w, h);
}

static void surface_frame(struct wl_client *client, struct wl_resource *resource,
                          uint32_t serial) {
    dcomp_surface_frame(client, resource, serial);
}

static void surface_set_opaque_region(struct wl_client *client,
                                      struct wl_resource *resource,
                                      struct wl_resource *region) {
    (void)client; (void)resource; (void)region;
}

static void surface_set_input_region(struct wl_client *client,
                                     struct wl_resource *resource,
                                     struct wl_resource *region) {
    (void)client; (void)resource; (void)region;
}

static void surface_set_buffer_scale(struct wl_client *client,
                                     struct wl_resource *resource,
                                     int32_t scale) {
    (void)client; (void)resource; (void)scale;
}

static void surface_set_buffer_transform(struct wl_client *client,
                                        struct wl_resource *resource,
                                        int32_t transform) {
    (void)client; (void)resource; (void)transform;
}

const struct wl_surface_interface surface_impl = {
    .destroy = surface_destroy,
    .attach = surface_attach,
    .damage = surface_damage,
    .frame = surface_frame,
    .set_opaque_region = surface_set_opaque_region,
    .set_input_region = surface_set_input_region,
    .set_buffer_scale = surface_set_buffer_scale,
    .set_buffer_transform = surface_set_buffer_transform,
};

static void compositor_create_surface(struct wl_client *client,
                                      struct wl_resource *resource,
                                      uint32_t id) {
    dcomp_create_surface(client, resource, id);
}

static void compositor_create_region(struct wl_client *client,
                                     struct wl_resource *resource,
                                     uint32_t id) {
    dcomp_create_region(client, resource, id);
}

const struct wl_compositor_interface compositor_impl = {
    .create_surface = compositor_create_surface,
    .create_region = compositor_create_region,
};

void destroy_wl_compositor(struct wl_resource *resource) {
    (void)resource;
}

void destroy_wl_surface(struct wl_resource *resource) {
    dcomp_surface_destroy(resource);
}

void dcomp_wl_compositor_init(struct dcomp_server *server) {
    struct wl_global *global = wl_global_create(server->display,
                                                &wl_compositor_interface, 4,
                                                server, NULL);
    if (!global) {
        fprintf(stderr, "failed to create compositor global\n");
        exit(1);
    }
    wl_global_set_user_data(global, server);
}

// xdg_shell (WM base)
static void xdg_wm_base_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

static void xdg_wm_base_create_xdg_surface(struct wl_client *client,
                                           struct wl_resource *resource,
                                           uint32_t id,
                                           struct wl_resource *surface) {
    dcomp_get_xdg_surface(client, resource, id, surface);
}

static void xdg_wm_base_create_positioner(struct wl_client *client,
                                         struct wl_resource *resource,
                                         uint32_t id) {
    (void)client; (void)resource; (void)id;
}

const struct xdg_wm_base_interface xdg_wm_base_impl = {
    .destroy = xdg_wm_base_destroy,
    .create_positioner = xdg_wm_base_create_positioner,
    .get_xdg_surface = xdg_wm_base_create_xdg_surface,
};

void dcomp_xdg_shell_init(struct dcomp_server *server) {
    struct wl_global *global = wl_global_create(server->display,
                                                &xdg_wm_base_interface, 1,
                                                server, NULL);
    if (!global) {
        fprintf(stderr, "failed to create xdg shell global\n");
        exit(1);
    }
    wl_global_set_user_data(global, server);
}

// seat
static void seat_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

static void seat_get_pointer(struct wl_client *client,
                             struct wl_resource *resource,
                             uint32_t id) {
    dcomp_get_pointer(client, resource, id);
}

static void seat_get_keyboard(struct wl_client *client,
                              struct wl_resource *resource,
                              uint32_t id) {
    dcomp_get_keyboard(client, resource, id);
}

static void seat_get_touch(struct wl_client *client,
                           struct wl_resource *resource,
                           uint32_t id) {
    (void)client; (void)resource; (void)id;
}

const struct wl_seat_interface seat_impl = {
    .get_pointer = seat_get_pointer,
    .get_keyboard = seat_get_keyboard,
    .get_touch = seat_get_touch,
};

void dcomp_wl_seat_init(struct dcomp_server *server) {
    struct wl_global *global = wl_global_create(server->display,
                                                &wl_seat_interface, 6,
                                                server, NULL);
    if (!global) {
        fprintf(stderr, "failed to create seat global\n");
        exit(1);
    }
    wl_global_set_user_data(global, server);
}

// wl_output
static void output_release(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

const struct wl_output_interface output_impl = {
    .release = output_release,
};

void dcomp_wl_output_init(struct dcomp_server *server) {
    struct wl_global *global = wl_global_create(server->display,
                                                &wl_output_interface, 4,
                                                server, NULL);
    if (!global) {
        fprintf(stderr, "failed to create output global\n");
        exit(1);
    }
    wl_global_set_user_data(global, server);
}
