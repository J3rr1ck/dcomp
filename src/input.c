#define _GNU_SOURCE
#include "input.h"
#include "compositor.h"
#include "desktop.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <xkbcommon/xkbcommon.h>

#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
#include <libinput.h>
#include <libudev.h>
#endif

extern volatile int dcomp_running;

struct dcomp_input *input_create(struct dcomp_server *server) {
    struct dcomp_input *in = calloc(1, sizeof(struct dcomp_input));
    in->server = server;

#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
    in->udev = udev_new();
    if (!in->udev) {
        fprintf(stderr, "failed to create udev context\n");
        free(in);
        return NULL;
    }

    in->li = libinput_udev_create_context(LIBINPUT_CONTEXT_BACKGROUND, NULL, in->udev);
    if (!in->li) {
        fprintf(stderr, "failed to create libinput context\n");
        udev_unref(in->udev);
        free(in);
        return NULL;
    }

    if (libinput_udev_assign_seat(in->li, "seat0") != 0) {
        fprintf(stderr, "failed to assign seat\n");
        libinput_unref(in->li);
        udev_unref(in->udev);
        free(in);
        return NULL;
    }
#else
    fprintf(stderr, "input: libinput not available, input disabled\n");
#endif

    // Initialize xkb
    in->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!in->xkb_ctx) {
        fprintf(stderr, "failed to create xkb context\n");
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
        libinput_unref(in->li);
        udev_unref(in->udev);
#endif
        free(in);
        return NULL;
    }

    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us",
    };
    in->xkb_keymap = xkb_keymap_new_from_names(in->xkb_ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!in->xkb_keymap) {
        fprintf(stderr, "failed to compile xkb keymap\n");
        xkb_context_unref(in->xkb_ctx);
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
        libinput_unref(in->li);
        udev_unref(in->udev);
#endif
        free(in);
        return NULL;
    }

    in->xkb_state = xkb_state_new(in->xkb_keymap);
    if (!in->xkb_state) {
        fprintf(stderr, "failed to create xkb state\n");
        xkb_keymap_unref(in->xkb_keymap);
        xkb_context_unref(in->xkb_ctx);
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
        libinput_unref(in->li);
        udev_unref(in->udev);
#endif
        free(in);
        return NULL;
    }

    in->serial = 0;
    in->cursor_x = 0;
    in->cursor_y = 0;
    in->button_pressed = false;
    return in;
}

void input_destroy(struct dcomp_input *in) {
    if (!in) return;
    if (in->xkb_state) xkb_state_unref(in->xkb_state);
    if (in->xkb_keymap) xkb_keymap_unref(in->xkb_keymap);
    if (in->xkb_ctx) xkb_context_unref(in->xkb_ctx);
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
    if (in->li) libinput_unref(in->li);
    if (in->udev) udev_unref(in->udev);
#endif
    free(in);
}

void input_set_keyboard_focus(struct dcomp_input *in, struct dcomp_view *view) {
    (void)in; (void)view;
}

void input_request_logout(struct dcomp_input *in) {
    (void)in;
    dcomp_running = 0;
}

void input_dispatch(struct dcomp_input *in) {
#if defined(HAVE_LIBINPUT) && defined(HAVE_UDEV)
    if (!in || !in->li) return;

    struct libinput_event *ev;
    while ((ev = libinput_get_event(in->li)) != NULL) {
        int type = libinput_event_get_type(ev);
        switch (type) {
        case LIBINPUT_EVENT_POINTER_MOTION: {
            double dx = libinput_event_pointer_get_dx(ev);
            double dy = libinput_event_pointer_get_dy(ev);
            in->cursor_x += (int32_t)dx;
            in->cursor_y += (int32_t)dy;
            struct dcomp_renderer *r = in->server->renderer;
            if (in->cursor_x < 0) in->cursor_x = 0;
            if (in->cursor_y < 0) in->cursor_y = 0;
            if (in->cursor_x >= (int32_t)r->output_w) in->cursor_x = r->output_w - 1;
            if (in->cursor_y >= (int32_t)r->output_h) in->cursor_y = r->output_h - 1;
            break;
        }
        case LIBINPUT_EVENT_POINTER_BUTTON: {
            uint32_t btn = libinput_event_pointer_get_button(ev);
            bool pressed = libinput_event_pointer_get_button_state(ev) == LIBINPUT_BUTTON_STATE_PRESSED;
            if (btn == 0x110 && pressed) {
                in->button_pressed = true;
                struct dcomp_desktop *d = in->server->desktop;
                if (d && in->cursor_y < (int32_t)d->panel_h) {
                    uint32_t bx = d->logout_btn_x;
                    uint32_t by = d->logout_btn_y;
                    uint32_t bw = d->logout_btn_w;
                    uint32_t bh = d->logout_btn_h;
                    if (in->cursor_x >= (int32_t)bx &&
                        in->cursor_x < (int32_t)(bx + bw) &&
                        in->cursor_y >= (int32_t)by &&
                        in->cursor_y < (int32_t)(by + bh)) {
                        input_request_logout(in);
                    }
                }
            } else {
                in->button_pressed = false;
            }
            break;
        }
        case LIBINPUT_EVENT_KEYBOARD_KEY:
            break;
        case LIBINPUT_EVENT_POINTER_AXIS:
            break;
        default:
            break;
        }
        libinput_event_destroy(ev);
    }
#else
    (void)in;
#endif
}
