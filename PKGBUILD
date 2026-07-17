# Maintainer: danger <danger@localhost>
pkgname=dcomp
pkgver=1.0.0
pkgrel=1
pkgdesc="A pure Vulkan Wayland compositor with minimal desktop environment"
arch=('x86_64')
url="https://github.com/example/dcomp"
license=('MIT')
depends=('wayland' 'vulkan-loader' 'libxkbcommon')
makedepends=('meson' 'ninja' 'glslang' 'vulkan-headers' 'wayland-protocols')
source=("${pkgname}-${pkgver}.tar.gz")
md5sums=('SKIP')
build() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    meson setup build --prefix=/usr
    meson compile -C build
}
package() {
    cd "${srcdir}/${pkgname}-${pkgver}"
    DESTDIR="${pkgdir}" meson install -C build
}
