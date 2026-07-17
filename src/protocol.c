#define _GNU_SOURCE
#include "protocol.h"
#include "compositor.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wayland-server.h>

// wl_buffer interface (minimal)
static void buffer_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

static const struct wl_buffer_interface buffer_impl = {
    .destroy = buffer_destroy,
};

void dcomp_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct dcomp_server *server = wl_resource_get_user_data(resource);
    struct wl_resource *surf_res = wl_resource_create(client, &wl_surface_interface, 4, id);
    if (!surf_res) return;

    struct dcomp_surface *surf = calloc(1, sizeof(struct dcomp_surface));
    surf->resource = surf_res;
    surf->scale = 1;
    wl_resource_set_user_data(surf_res, server);
    wl_resource_set_implementation(surf_res, &surface_impl, surf, destroy_wl_surface);
    wl_list_insert(&server->surfaces, &surf->link);
}

void dcomp_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    (void)client; (void)resource; (void)id;
}

void dcomp_surface_destroy(struct wl_resource *resource) {
    struct dcomp_surface *surf = wl_resource_get_user_data(resource);
    if (!surf) return;
    wl_list_remove(&surf->link);
    free(surf);
}

void dcomp_surface_attach(struct wl_client *client, struct wl_resource *resource,
                          struct wl_resource *buffer, int32_t x, int32_t y) {
    (void)client; (void)x; (void)y;
    struct dcomp_surface *surf = wl_resource_get_user_data(resource);
    if (!surf) return;
    surf->buffer_resource = buffer;
    surf->has_buffer = true;
}

void dcomp_surface_damage(struct wl_client *client, struct wl_resource *resource,
                          int32_t x, int32_t y, int32_t w, int32_t h) {
    (void)client; (void)resource; (void)x; (void)y; (void)w; (void)h;
}

void dcomp_surface_frame(struct wl_client *client, struct wl_resource *resource,
                         uint32_t serial) {
    (void)client; (void)resource; (void)serial;
}

// xdg_surface
struct dcomp_xdg_surface {
    struct wl_resource *resource;
    struct dcomp_surface *surf;
    char *title;
};

static void xdg_surface_destroy(struct wl_client *client, struct wl_resource *resource) {
    (void)client;
    struct dcomp_xdg_surface *xsurf = wl_resource_get_user_data(resource);
    if (!xsurf) return;
    free(xsurf->title);
    free(xsurf);
}

static void xdg_surface_get_toplevel(struct wl_client *client,
                                     struct wl_resource *resource,
                                     uint32_t id) {
    (void)resource;
    struct wl_resource *tl_res = wl_resource_create(client, &xdg_toplevel_interface, 1, id);
    if (!tl_res) return;
}

static void xdg_surface_set_title(struct wl_client *client,
                                  struct wl_resource *resource,
                                  const char *title) {
    dcomp_xdg_surface_set_title(client, resource, title);
}

static const struct xdg_surface_interface xdg_surface_impl = {
    .destroy = xdg_surface_destroy,
    .get_toplevel = xdg_surface_get_toplevel,
};

void dcomp_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                           struct wl_resource *surface) {
    struct dcomp_server *server = wl_resource_get_user_data(resource);
    struct wl_resource *xres = wl_resource_create(client, &xdg_surface_interface, 1, id);
    if (!xres) return;

    struct dcomp_xdg_surface *xsurf = calloc(1, sizeof(struct dcomp_xdg_surface));
    xsurf->resource = xres;
    xsurf->surf = wl_resource_get_user_data(surface);
    wl_resource_set_implementation(xres, &xdg_surface_impl, xsurf, NULL);

    if (server->desktop && xsurf->surf) {
        desktop_add_view(server->desktop, xsurf->surf);
    }
}

void dcomp_xdg_surface_set_title(struct wl_client *client,
                                 struct wl_resource *resource,
                                 const char *title) {
    (void)client;
    struct dcomp_xdg_surface *xsurf = wl_resource_get_user_data(resource);
    if (!xsurf) return;
    free(xsurf->title);
    xsurf->title = strdup(title);
}

// pointer
struct dcomp_pointer {
    struct wl_resource *resource;
    struct dcomp_server *server;
};

static void pointer_release(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

static const struct wl_pointer_interface pointer_impl = {
    .release = pointer_release,
};

void dcomp_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct dcomp_server *server = wl_resource_get_user_data(resource);
    struct wl_resource *p_res = wl_resource_create(client, &wl_pointer_interface, 6, id);
    if (!p_res) return;
    struct dcomp_pointer *ptr = calloc(1, sizeof(struct dcomp_pointer));
    ptr->resource = p_res;
    ptr->server = server;
    wl_resource_set_implementation(p_res, &pointer_impl, ptr, NULL);
}

// keyboard
struct dcomp_keyboard {
    struct wl_resource *resource;
    struct dcomp_server *server;
};

static void keyboard_release(struct wl_client *client, struct wl_resource *resource) {
    (void)client; (void)resource;
}

static const struct wl_keyboard_interface keyboard_impl = {
    .release = keyboard_release,
};

void dcomp_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id) {
    struct dcomp_server *server = wl_resource_get_user_data(resource);
    struct wl_resource *k_res = wl_resource_create(client, &wl_keyboard_interface, 6, id);
    if (!k_res) return;
    struct dcomp_keyboard *kb = calloc(1, sizeof(struct dcomp_keyboard));
    kb->resource = k_res;
    kb->server = server;
    wl_resource_set_implementation(k_res, &keyboard_impl, kb, NULL);
}
