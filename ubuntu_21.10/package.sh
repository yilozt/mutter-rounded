#!/bin/bash

dir="$(cd $(dirname $0); pwd)"
aur="${dir}/../aur"
cd "${dir}"

SUDO=sudo
if [ "$(whoami)" = "root" ]; then
  SUDO=""
fi

mkdir -p workspace
cd workspace

# enable deb-src source, install dependencies
${SUDO} sed -i 's/^# deb-src/deb-src/g' /etc/apt/sources.list
${SUDO} apt update
${SUDO} apt install -y gnupg pbuilder ubuntu-dev-tools apt-file
${SUDO} apt build-dep -y mutter

# download the source code
[ -d mutter-40.5 ] && rm -rf mutter-40.5
pull-lp-source mutter impish
wget -nc https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/40.5/src/shell-blur-effect.c
wget -nc https://gitlab.gnome.org/GNOME/gnome-shell/-/raw/40.5/src/shell-blur-effect.h

# patch the code
cp shell*.[ch] mutter-40.5/src
cp "$aur"/*.[ch] mutter-40.5/src
cd mutter-40.5/
patch -p1 < "${aur}"/rounded_corners_40.5.patch
patch -p1 < "${aur}"/shell_blur_effect_40.5.patch
patch -p1 < "${dir}"/symbols.patch

# commit changes
dpkg-source --commit . rounded_corners

# build packages
dpkg-buildpackage -B

mv *.deb ../
