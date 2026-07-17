#ifndef INPUT_H
#define INPUT_H

struct dcomp_view;  // forward declaration from compositor.h

#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>
#include <stdbool.h>

struct dcomp_input {
    struct dcomp_server *server;
    struct wl_resource *seat_resource;

    // keyboard
    struct xkb_context *xkb_ctx;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;

    uint32_t serial;
};

struct dcomp_input *input_create(struct dcomp_server *server);
void input_destroy(struct dcomp_input *in);
void input_set_keyboard_focus(struct dcomp_input *in, struct dcomp_view *view);

#endif
