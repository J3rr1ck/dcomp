project('dcomp', 'c',
          version : '1.0.0',
          default_options : ['warning_level=3', 'c_std=c11'])

wayland_server = dependency('wayland-server')
wayland_client = dependency('wayland-client')
wayland_protocols = dependency('wayland-protocols', required: false)
vulkan = dependency('vulkan')
xkbcommon = dependency('xkbcommon')


# Wayland scanner
wayland_scanner = find_program('wayland-scanner')

# XDG shell XML from wayland-protocols
wl_protocols_dir = wayland_protocols.get_pkgconfig_variable('pkgdatadir',
    default: '/usr/share/wayland-protocols')
xdg_shell_xml = wl_protocols_dir / 'stable' / 'xdg-shell' / 'xdg-shell.xml'

message('XDG shell XML: ' + xdg_shell_xml)

# Generate protocol code
xdg_shell_c = custom_target('xdg-shell-c',
    input : xdg_shell_xml,
    output : 'xdg-shell.c',
    command : [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_h = custom_target('xdg-shell-h',
    input : xdg_shell_xml,
    output : 'xdg-shell.h',
    command : [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
)

# Shader compilation
glslang = find_program('glslangValidator', required: true)
vert_spv = custom_target('vert-spv',
    input : 'src/shaders/vert.vert',
    output : 'vert.spv',
    command : [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'],
)
frag_spv = custom_target('frag-spv',
    input : 'src/shaders/frag.frag',
    output : 'frag.spv',
    command : [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'],
)

# Embed shaders into C headers
embed_tool = executable('embed',
    'tools/embed.c',
    native : true,
)

vert_h = custom_target('vert-h',
    input : vert_spv,
    output : 'vert.h',
    command : [embed_tool, '@INPUT@', '@OUTPUT@'],
    build_by_default : true,
)

frag_h = custom_target('frag-h',
    input : frag_spv,
    output : 'frag.h',
    command : [embed_tool, '@INPUT@', '@OUTPUT@'],
    build_by_default : true,
)

# Compositor executable
dcomp_src = [
    'src/main.c',
    'src/compositor.c',
    'src/renderer.c',
    'src/desktop.c',
    'src/input.c',
    'src/protocol.c',
]

dcomp = executable('dcomp',
    dcomp_src,
    xdg_shell_c, xdg_shell_h,
    vert_h, frag_h,
    dependencies : [wayland_server, vulkan, xkbcommon],
    include_directories : [include_directories('.')],
    install : true,
)

# Panel (stub, not built by default)
# panel_src = ['src/panel.c']
# panel = executable('dcomp-panel',
#     panel_src,
#     dependencies : [wayland_client, vulkan],
#     install : true,
# )
