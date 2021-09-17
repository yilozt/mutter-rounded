# Maintainer: Luo Yi <langisme_at_qq_dot_com>

# Contributor: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

# Contributor: Ionut Biru <ibiru@archlinux.org>
# Contributor: Michael Kanis <mkanis_at_gmx_dot_de>

pkgname=mutter-rounded
pkgver=40.4
pkgrel=1
pkgdesc="A window manager for GNOME, with rounded corners patch"
url="https://gitlab.gnome.org/GNOME/mutter"
arch=(x86_64)
license=(GPL)
depends=(dconf gobject-introspection-runtime gsettings-desktop-schemas
         libcanberra startup-notification zenity libsm gnome-desktop upower
         libxkbcommon-x11 gnome-settings-daemon libgudev libinput pipewire
         xorg-xwayland graphene libxkbfile)
makedepends=(gobject-introspection git egl-wayland meson xorg-server)
checkdepends=(xorg-server-xvfb pipewire-media-session)
provides=(mutter libmutter-8.so)
conflicts=(mutter)
groups=(gnome)
install=mutter.install

_commit=2bfef7dbdc6f432a5433c93c1fcdbf00099367c8  # tags/40.4^0
_mutter_src="$pkgname::git+https://gitlab.gnome.org/GNOME/mutter.git#commit=$_commit"
_setting_src="mutter_setting::https://gitlab.gnome.org/lluo/mutter-rounded-setting/uploads/7370b166a10976a0846b57a5ebbe2737/main.js"

if [ "${LANG}" == "zh_CN.UTF-8" ] ; then
  _mutter_src="$pkgname::git+https://gitee.com/mirrors_GNOME/mutter.git#commit=$_commit"
  _setting_src="mutter_setting::https://gitee.com/lluo/mutter-rounded-setting/attach_files/833785/download/main.js"
fi

source=("$_mutter_src"
        "rounded_corners_${pkgver}.patch"
        "meta_clip_effect.c"
        "meta_clip_effect.h"
        "$_setting_src")
sha256sums=('SKIP'
            '993cb349226afe198771bdca32c225d1bf663b2b14a3454270fa0b64f4e19cab'
            '1d4757a46db018f0ac080787c372a01f563499a19c6315fd1b4c3610f450b041'
            '2a4670913601b97f809a486da7c11b4e14472f62211154d5a417d3b3e4d77859'
            '08a3dbbea7205cdc422901f9b78fd5fbe95bf7378197f33a07bf70a342f35dc5')

pkgver() {
  cd $pkgname
  git describe --tags | sed 's/-/+/g'
}

prepare() {
  sed -i '1i\#!/usr/bin/gjs' mutter_setting

  cd $pkgname
  cp $srcdir/meta_clip_effect.[ch] $srcdir/$pkgname/src
  patch -p1 < $srcdir/rounded_corners_${pkgver}.patch
}

build() {
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
