# Maintainer: Luo Yi <langisme_at_qq_dot_com>

# Contributor: Jan Alexander Steffens (heftig) <heftig@archlinux.org>

# Contributor: Ionut Biru <ibiru@archlinux.org>
# Contributor: Michael Kanis <mkanis_at_gmx_dot_de>

pkgname=mutter-rounded
pkgver=40.5
pkgrel=1
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
_setting_src="mutter_setting::https://gitlab.gnome.org/lluo/mutter-rounded-setting/uploads/b1ba5b74faf000b93a4ddcbd73ddcb98/main.js"

if [ "${LANG}" = "zh_CN.UTF-8" ] ; then
  _mutter_src="$pkgname::git+https://gitee.com/mirrors_GNOME/mutter.git#commit=$_commit"
  _setting_src="mutter_setting::https://gitee.com/lluo/mutter-rounded-setting/attach_files/847740/download/main.js"
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
            '2502b64ecff2ac051b3e1cda5d8c424ca0caa71aa5055a48ba2e8441358c6ab1'
            'c9eb42bdb6f8f1dbed2d6e6d1c47f5826f308411e91838eb407ba2aeab98b535'
            'c47f3a998e5b8dbb53951be450a5f561f91ad8803a653dab2bcee69b1400e6d4'
            '2ec553a260497f0ac0180512201c9819b10159a15fcbc6d5007932d8e2a44844'
            'a02e991156dc3b4418899b73a2e65187a43990851fb235ea128ed7650c839a3b'
            '8fb024306843153b28db2f5347775ef7e8add1dd846345148a572ad5336e168b'
            'd58056b5028e1cf02a029036792f52e3429bd5f71a9403b5be93d95a7ba8252a'
            '0182ba6d60c1bcc13d959b018662ea0bc1cc2347e6648fd455212fa3221dfd79')

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
