#include "input.h"
#include "compositor.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <xkbcommon/xkbcommon.h>

struct dcomp_input *input_create(struct dcomp_server *server) {
    struct dcomp_input *in = calloc(1, sizeof(struct dcomp_input));
    in->server = server;

    // Initialize xkb
    in->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!in->xkb_ctx) {
        fprintf(stderr, "failed to create xkb context\n");
        free(in);
        return NULL;
    }

    // Load default keymap from system
    struct xkb_rule_names names = {
        .rules = "evdev",
        .model = "pc105",
        .layout = "us",
    };
    in->xkb_keymap = xkb_keymap_new_from_names(in->xkb_ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!in->xkb_keymap) {
        fprintf(stderr, "failed to compile xkb keymap\n");
        xkb_context_unref(in->xkb_ctx);
        free(in);
        return NULL;
    }

    in->xkb_state = xkb_state_new(in->xkb_keymap);
    if (!in->xkb_state) {
        fprintf(stderr, "failed to create xkb state\n");
        xkb_keymap_unref(in->xkb_keymap);
        xkb_context_unref(in->xkb_ctx);
        free(in);
        return NULL;
    }

    in->serial = 0;
    return in;
}

void input_destroy(struct dcomp_input *in) {
    if (!in) return;
    if (in->xkb_state) xkb_state_unref(in->xkb_state);
    if (in->xkb_keymap) xkb_keymap_unref(in->xkb_keymap);
    if (in->xkb_ctx) xkb_context_unref(in->xkb_ctx);
    free(in);
}

void input_set_keyboard_focus(struct dcomp_input *in, struct dcomp_view *view) {
    // Send keyboard focus to client
    (void)in; (void)view;
}
