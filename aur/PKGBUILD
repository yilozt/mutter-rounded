# Maintainer: Luo Yi <langisme_at_qq_dot_com>

# Contributor: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

# Contributor: Ionut Biru <ibiru@archlinux.org>
# Contributor: Michael Kanis <mkanis_at_gmx_dot_de>

pkgname=mutter-rounded
pkgver=40.5
pkgrel=1.2
pkgdesc="A window manager for GNOME, with rounded corners patch"
url="https://gitlab.gnome.org/GNOME/mutter"
arch=(x86_64)
license=(GPL)
depends=(dconf gobject-introspection-runtime gsettings-desktop-schemas
         libcanberra startup-notification zenity libsm gnome-desktop upower
         libxkbcommon-x11 gnome-settings-daemon libgudev libinput pipewire
         xorg-xwayland graphene libxkbfile)
makedepends=(gobject-introspection git egl-wayland meson xorg-server
             wayland-protocols)
checkdepends=(xorg-server-xvfb pipewire-media-session)
provides=(mutter libmutter-8.so)
conflicts=(mutter)
groups=(gnome)
install=mutter.install

_commit=2b2b3ab8502a5bcc2436e169279d2421f6f1a605  # tags/40.5^0
_mutter_src="$pkgname::git+https://gitlab.gnome.org/GNOME/mutter.git#commit=$_commit"
_shell_blur_h_src="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/${pkgver}/src/shell-blur-effect.h"
_shell_blur_c_src="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/${pkgver}/src/shell-blur-effect.c"
_setting_src="mutter_setting::https://gitlab.gnome.org/lluo/mutter-rounded-setting/uploads/8f8e3f8d39f31e602c2d09884a6c5dd1/main.js"

if [ "${LANG}" = "zh_CN.UTF-8" ] ; then
  _mutter_src="$pkgname::git+https://gitee.com/mirrors_GNOME/mutter.git#commit=$_commit"
  _setting_src="mutter_setting::https://gitee.com/lluo/mutter-rounded-setting/attach_files/865566/download/main.js"
fi

source=("$_mutter_src"
        "rounded_corners_${pkgver}.patch"
        "shell_blur_effect_${pkgver}.patch"
        "meta_clip_effect.c"
        "meta_clip_effect.h"
        "shader.h"
        "$_shell_blur_h_src"
        "$_shell_blur_c_src"
        "$_setting_src")
sha256sums=('SKIP'
            '0c2fc381c7529d012d3d8a4368941db7b60ce6128005008b5ddfb4da16dc2b83'
            '895f35f5e8a458c71b4312061cf7d2b0108a3c6df4b0324ab342c5a3576ee09a'
            '622ec46c9a1e8e675adbc3d852467e46c8d3101caa0260bf63b4c1a1b87a24a1'
            '2ec553a260497f0ac0180512201c9819b10159a15fcbc6d5007932d8e2a44844'
            'a02e991156dc3b4418899b73a2e65187a43990851fb235ea128ed7650c839a3b'
            '8fb024306843153b28db2f5347775ef7e8add1dd846345148a572ad5336e168b'
            'd58056b5028e1cf02a029036792f52e3429bd5f71a9403b5be93d95a7ba8252a'
            'c0eff82301060044d231f0b674025e5a00d1152e515e08d16fd18363da5187e5')

pkgver() {
  cd $pkgname
  git describe --tags | sed 's/-/+/g'
}

prepare() {
  sed -i '1i\#!/usr/bin/gjs' mutter_setting

  cd $pkgname
  cp $srcdir/*.[ch] $srcdir/$pkgname/src
  patch -p1 < $srcdir/rounded_corners_${pkgver}.patch
  patch -p1 < $srcdir/shell_blur_effect_${pkgver}.patch
}

build() {
  echo "skip" > /dev/null
  CFLAGS="${CFLAGS/-O2/-O3} -fno-semantic-interposition"
  LDFLAGS+=" -Wl,-Bsymbolic-functions"
  arch-meson $pkgname build \
    -D egl_device=true \
    -D wayland_eglstream=true \
    -D installed_tests=false \
    -D profiler=false
  meson compile -C build
}

_check() (
  mkdir -p -m 700 "${XDG_RUNTIME_DIR:=$PWD/runtime-dir}"
  glib-compile-schemas "${GSETTINGS_SCHEMA_DIR:=$PWD/build/data}"
  export XDG_RUNTIME_DIR GSETTINGS_SCHEMA_DIR

  pipewire &
  _p1=$!

  pipewire-media-session &
  _p2=$!

  trap "kill $_p1 $_p2; wait" EXIT

  meson test -C build --print-errorlogs
)

check() {
  dbus-run-session xvfb-run \
    -s '-screen 0 1920x1080x24 -nolisten local +iglx -noreset' \
    bash -c "$(declare -f _check); _check"
}

package() {
  meson install -C build --destdir "$pkgdir"
  install mutter_setting $pkgdir/usr/bin/
}
