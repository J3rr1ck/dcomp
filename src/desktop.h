#ifndef DESKTOP_H
#define DESKTOP_H

struct dcomp_surface;  // forward declaration from compositor.h

#include "compositor.h"

struct dcomp_desktop {
    struct dcomp_server *server;
    struct wl_list views; // struct dcomp_view

    // background texture
    struct dcomp_texture *bg_tex;
    uint32_t bg_w, bg_h;

    // panel (top bar)
    struct dcomp_view *panel_view;
    struct dcomp_texture *panel_tex;
    uint32_t panel_h;

    // active window tracking
    struct dcomp_view *active_view;
};

struct dcomp_desktop *desktop_create(struct dcomp_server *server);
void desktop_destroy(struct dcomp_desktop *d);
void desktop_layout(struct dcomp_desktop *d);
void desktop_draw(struct dcomp_desktop *d);
void desktop_add_view(struct dcomp_desktop *d, struct dcomp_surface *surf);
void desktop_remove_view(struct dcomp_desktop *d, struct dcomp_surface *surf);

#endif
