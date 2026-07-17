#ifndef INPUT_H
#define INPUT_H

struct dcomp_view;  // forward declaration from compositor.h

#include <wayland-server.h>
#include <xkbcommon/xkbcommon.h>
#include <stdbool.h>
#include "config.h"

#if HAVE_LIBINPUT && HAVE_UDEV
#include <libinput.h>
#include <libudev.h>
#endif

struct dcomp_input {
    struct dcomp_server *server;
    struct wl_resource *seat_resource;

#if HAVE_LIBINPUT && HAVE_UDEV
    struct libinput *li;
    struct udev *udev;
#endif

    struct xkb_context *xkb_ctx;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;

    int32_t cursor_x, cursor_y;
    bool button_pressed;

    uint32_t serial;
};

struct dcomp_input *input_create(struct dcomp_server *server);
void input_destroy(struct dcomp_input *in);
void input_set_keyboard_focus(struct dcomp_input *in, struct dcomp_view *view);
void input_dispatch(struct dcomp_input *in);
void input_request_logout(struct dcomp_input *in);

#endif
