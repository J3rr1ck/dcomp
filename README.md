# dcomp - Pure Vulkan Wayland Compositor

A minimal Wayland compositor with a basic desktop environment, written from scratch using pure Vulkan (no wlroots, pango, or cairo).

## Features

- Wayland compositor implementing `wl_compositor`, `wl_surface`, `xdg_wm_base`, `wl_seat`, `wl_output`
- Vulkan renderer with swapchain, full-screen compositing
- Desktop background with gradient
- Simple top panel bar
- Window management (tile layout)
- Keyboard input via xkbcommon
- Pointer input support

## Requirements

- `wayland` (server + client)
- `vulkan-loader`
- `vulkan-headers`
- `libxkbcommon`
- `libxcb`, `xcb-util-wm`
- `wayland-protocols`
- `glslang` (for shader compilation)
- `meson`, `ninja` (build system)

## Build

```bash
meson setup build
meson compile -C build
```

## Run

```bash
./build/src/dcomp
```

Set `WAYLAND_DISPLAY` to the socket path (printed on startup).

## AUR Package

A `PKGBUILD` is included for Arch Linux packaging. Use `makepkg` to build.

## Structure

```
dcomp/
├── PKGBUILD
├── meson.build
├── README.md
├── tools/
│   └── embed.c
├── src/
│   ├── main.c          # Entry point
│   ├── compositor.c    # Wayland compositor globals
│   ├── compositor.h
│   ├── renderer.c      # Vulkan renderer
│   ├── renderer.h
│   ├── desktop.c       # Desktop environment
│   ├── desktop.h
│   ├── input.c         # Input handling (keyboard)
│   ├── input.h
│   ├── protocol.c      # Wayland protocol handlers
│   ├── protocol.h
│   ├── panel.c         # Desktop panel client
│   ├── panel.h
│   └── shaders/
│       ├── vert.glsl
│       └── frag.glsl
```

## License

MIT
