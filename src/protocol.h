#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <wayland-server.h>
#include "compositor.h"

// wl_compositor handlers
void dcomp_create_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id);
void dcomp_create_region(struct wl_client *client, struct wl_resource *resource, uint32_t id);

// wl_surface handlers
void dcomp_surface_destroy(struct wl_resource *resource);
void dcomp_surface_attach(struct wl_client *client, struct wl_resource *resource,
                          struct wl_resource *buffer, int32_t x, int32_t y);
void dcomp_surface_damage(struct wl_client *client, struct wl_resource *resource,
                          int32_t x, int32_t y, int32_t w, int32_t h);
void dcomp_surface_frame(struct wl_client *client, struct wl_resource *resource,
                         uint32_t serial);

// xdg_shell handlers
void dcomp_get_xdg_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id,
                           struct wl_resource *surface);
void dcomp_xdg_surface_set_title(struct wl_client *client, struct wl_resource *resource,
                                 const char *title);

// wl_seat handlers
void dcomp_get_pointer(struct wl_client *client, struct wl_resource *resource, uint32_t id);
void dcomp_get_keyboard(struct wl_client *client, struct wl_resource *resource, uint32_t id);

#endif
