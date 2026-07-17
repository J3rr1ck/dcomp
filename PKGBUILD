# Maintainer: danger <danger@localhost>
pkgname=dcomp
pkgver=1.0.0
pkgrel=1
pkgdesc="A pure Vulkan Wayland compositor with minimal desktop environment"
arch=('x86_64')
url="https://github.com/J3rr1ck/dcomp"
license=('MIT')
depends=('wayland' 'vulkan-icd-loader' 'libxkbcommon' 'libinput' 'systemd-libs')
makedepends=('meson' 'ninja' 'glslang' 'vulkan-headers' 'wayland-protocols' 'systemd')
source=("git+https://github.com/J3rr1ck/dcomp.git")
md5sums=('SKIP')
build() {
    cd "${srcdir}/${pkgname}"
    meson setup build --prefix=/usr
    meson compile -C build
}
package() {
    cd "${srcdir}/${pkgname}"
    DESTDIR="${pkgdir}" meson install -C build
}
