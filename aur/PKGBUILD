# Maintainer: Luo Yi <langisme_at_qq_dot_com>

# Contributor: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

# Contributor: Ionut Biru <ibiru@archlinux.org>
# Contributor: Michael Kanis <mkanis_at_gmx_dot_de>

pkgname=mutter-rounded
pkgver=41.2
pkgrel=2.1
pkgdesc="A window manager for GNOME, with rounded corners patch"
url="https://gitlab.gnome.org/GNOME/mutter"
arch=(x86_64)
license=(GPL)
depends=(dconf gobject-introspection-runtime gsettings-desktop-schemas
         libcanberra startup-notification zenity libsm gnome-desktop upower
         libxkbcommon-x11 gnome-settings-daemon libgudev libinput pipewire
         xorg-xwayland graphene libxkbfile libsysprof-capture)
makedepends=(gobject-introspection git egl-wayland meson xorg-server
             wayland-protocols sysprof)
checkdepends=(xorg-server-xvfb wireplumber python-dbusmock)
provides=(libmutter-9.so mutter)
conflicts=(mutter)
groups=(gnome)
install=mutter.install

_commit=664ac09eecfd365b5258f53d2c9e6c8410a37919  # tags/41.2^0
_mutter_src="$pkgname::git+https://gitlab.gnome.org/GNOME/mutter.git#commit=$_commit"
_shell_blur_h_src="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/41.1/src/shell-blur-effect.h"
_shell_blur_c_src="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/41.1/src/shell-blur-effect.c"
_settings_src="mutter_settings::git+https://github.com/yilozt/mutter-rounded-setting"

# Mirrors in Gitee
if [ "${LANG}" = "zh_CN.UTF-8" ] ; then
  _mutter_src="$pkgname::git+https://gitee.com/mirrors_GNOME/mutter.git#commit=$_commit"
  _settings_src="mutter_settings::git+https://gitee.com/lluo/mutter-rounded-setting/"
fi

source=("$_mutter_src"
        "$_settings_src"
        "rounded_corners.patch"
        "shell_blur_effect.patch"
        "meta_clip_effect.c"
        "meta_clip_effect.h"
        "shader.h"
        "$_shell_blur_h_src"
        "$_shell_blur_c_src"
        )
sha256sums=('SKIP'
            'SKIP'
            'f161768e446deb36ea787f03daf3162b517f091a1690b1938c99b777687f44f0'
            '895f35f5e8a458c71b4312061cf7d2b0108a3c6df4b0324ab342c5a3576ee09a'
            'dc4279ec3c0995ba9a9ee3ecd581376140c78089a058863dea0bd9ec7894dc49'
            '2ec553a260497f0ac0180512201c9819b10159a15fcbc6d5007932d8e2a44844'
            '644ab34d1c6eebe8d632b3b26a993a2b9f8a5b58c821758c50eef1cdd0b7cc0c'
            '8fb024306843153b28db2f5347775ef7e8add1dd846345148a572ad5336e168b'
            'd58056b5028e1cf02a029036792f52e3429bd5f71a9403b5be93d95a7ba8252a')

pkgver() {
  cd $pkgname
  git describe --tags | sed 's/[^-]*-g/r&/;s/-/+/g'
}

prepare() {
  sed -i '1i\#!/usr/bin/gjs' mutter_settings/dist/mutter_settings.js
  mv mutter_settings/dist/mutter_settings.js mutter_settings/dist/mutter_settings

  cd $pkgname
  cp $srcdir/*.[ch] $srcdir/$pkgname/src
  patch -p1 < $srcdir/rounded_corners.patch
  patch -p1 < $srcdir/shell_blur_effect.patch
}

build() {
  CFLAGS="${CFLAGS/-O2/-O3} -fno-semantic-interposition"
  LDFLAGS+=" -Wl,-Bsymbolic-functions"
  arch-meson $pkgname build \
    -D egl_device=true \
    -D wayland_eglstream=true \
    -D installed_tests=false
  meson compile -C build
}

_check() (
  mkdir -p -m 700 "${XDG_RUNTIME_DIR:=$PWD/runtime-dir}"
  glib-compile-schemas "${GSETTINGS_SCHEMA_DIR:=$PWD/build/data}"
  export XDG_RUNTIME_DIR GSETTINGS_SCHEMA_DIR

  pipewire &
  _p1=$!

  wireplumber &
  _p2=$!

  trap "kill $_p1 $_p2; wait" EXIT

  meson test -C build --print-errorlogs
)

check() {
  dbus-run-session xvfb-run -s '-nolisten local +iglx -noreset' \
    bash -c "$(declare -f _check); _check"
}

package() {
  meson install -C build --destdir "$pkgdir"
  install mutter_settings/dist/mutter_settings $pkgdir/usr/bin/

  _uuid=pickawindow@lluo.gitlab.com
  _schemas=org.gnome.shell.extensions.pickawindow.gschema.xml
  
  install -d "$pkgdir/usr/share/gnome-shell/extensions/$_uuid"
  cp -r mutter_settings/$_uuid/* "$pkgdir/usr/share/gnome-shell/extensions/$_uuid/"

  install -d "$pkgdir/usr/share/glib-2.0/schemas/"
  ln -s "/usr/share/gnome-shell/extensions/$_uuid/schemas/$_schemas" \
    "$pkgdir/usr/share/glib-2.0/schemas/"
}
