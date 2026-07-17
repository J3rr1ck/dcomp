#include "desktop.h"
#include "compositor.h"
#include "renderer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void render_panel(struct dcomp_desktop *d) {
    uint32_t w = d->server->renderer->output_w;
    uint32_t h = d->panel_h;
    uint32_t *pixels = calloc(w * h, 4);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            pixels[y * w + x] = 0x202020FF;
        }
    }
    // Draw a logout button at the right side (red square with white X)
    uint32_t btn_size = h - 4;
    uint32_t btn_x = w - btn_size - 4;
    uint32_t btn_y = 2;
    d->logout_btn_x = btn_x;
    d->logout_btn_y = btn_y;
    d->logout_btn_w = btn_size;
    d->logout_btn_h = btn_size;

    for (uint32_t by = btn_y; by < btn_y + btn_size; by++) {
        for (uint32_t bx = btn_x; bx < btn_x + btn_size; bx++) {
            pixels[by * w + bx] = 0xCC2222FF; // red background
        }
    }
    // Draw a white X
    uint32_t cx = btn_x + btn_size / 2;
    uint32_t cy = btn_y + btn_size / 2;
    for (int32_t i = -3; i <= 3; i++) {
        uint32_t x1 = cx + i;
        uint32_t y1 = cy + i;
        uint32_t x2 = cx + i;
        uint32_t y2 = cy - i;
        if (x1 < w && y1 < h) pixels[y1 * w + x1] = 0xFFFFFFFF;
        if (x2 < w && y2 < h) pixels[y2 * w + x2] = 0xFFFFFFFF;
    }

    d->panel_tex = renderer_create_texture(d->server->renderer, w, h, pixels);
    free(pixels);
}

static void render_background(struct dcomp_desktop *d) {
    uint32_t w = d->server->renderer->output_w;
    uint32_t h = d->server->renderer->output_h;
    uint32_t *pixels = calloc(w * h, 4);
    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            uint8_t r = (255 * y) / h;
            uint8_t g = (128 * x) / w + 64;
            uint8_t b = 255 - (255 * y) / h;
            pixels[y * w + x] = (r << 16) | (g << 8) | b | 0xFF000000;
        }
    }
    d->bg_tex = renderer_create_texture(d->server->renderer, w, h, pixels);
    free(pixels);
}

struct dcomp_desktop *desktop_create(struct dcomp_server *server) {
    struct dcomp_desktop *d = calloc(1, sizeof(struct dcomp_desktop));
    d->server = server;
    wl_list_init(&d->views);
    d->panel_h = 32;
    d->logout_btn_x = 0;
    d->logout_btn_y = 0;
    d->logout_btn_w = 0;
    d->logout_btn_h = 0;
    render_background(d);
    render_panel(d);
    return d;
}

void desktop_destroy(struct dcomp_desktop *d) {
    if (!d) return;
    renderer_destroy_texture(d->server->renderer, d->bg_tex);
    renderer_destroy_texture(d->server->renderer, d->panel_tex);
    free(d);
}

void desktop_layout(struct dcomp_desktop *d) {
    int32_t y = d->panel_h;
    struct dcomp_view *view;
    wl_list_for_each(view, &d->views, link) {
        view->x = 20;
        view->y = y;
        view->width = 800;
        view->height = 600;
        y += 20 + view->height;
        if (y > 1200) y = d->panel_h;
    }
}

void desktop_draw(struct dcomp_desktop *d) {
    struct dcomp_renderer *r = d->server->renderer;
    renderer_draw_view(r, d->bg_tex, 0, 0, r->output_w, r->output_h);
    renderer_draw_view(r, d->panel_tex, 0, 0, r->output_w, d->panel_h);
    struct dcomp_view *view;
    wl_list_for_each(view, &d->views, link) {
        if (!view->surface || !view->surface->has_buffer) continue;
    }
}

void desktop_add_view(struct dcomp_desktop *d, struct dcomp_surface *surf) {
    struct dcomp_view *view = calloc(1, sizeof(struct dcomp_view));
    view->surface = surf;
    view->mapped = true;
    wl_list_insert(&d->views, &view->link);
    surf->view = view;
    desktop_layout(d);
}

void desktop_remove_view(struct dcomp_desktop *d, struct dcomp_surface *surf) {
    (void)d;
    if (!surf->view) return;
    struct dcomp_view *view = surf->view;
    wl_list_remove(&view->link);
    free(view);
    surf->view = NULL;
}
