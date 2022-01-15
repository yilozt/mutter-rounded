#!/bin/bash

# The source of blur effect. You can replace with:
#   - https://gitee.com/mirrors_GNOME/gnome-shell/raw
#   - https://gitlab.gnome.org/GNOME/gnome-shell/-/raw
blur_effect_url="https://gitee.com/mirrors_GNOME/gnome-shell/raw"

dir="$(cd $(dirname $0); pwd)"
aur="${dir}/../aur"
cd "${dir}"
tool="$dir/../tool"

SUDO=sudo
if [ "$(whoami)" = "root" ]; then
  SUDO=""
fi

export LANG=en_US.UTF-8
. $tool

run mkdir -p workspace
run cd workspace

# enable deb-src source, install dependencies
run ${SUDO} sed -i 's/^# deb-src/deb-src/g' /etc/apt/sources.list
run ${SUDO} apt update
run ${SUDO} apt install -y gnupg pbuilder ubuntu-dev-tools apt-file
run ${SUDO} apt build-dep -y mutter

# download the source code
[ -d mutter-40.5 ] && run rm -rf mutter-40.5
run pull-lp-source mutter impish
run wget -nc ${blur_effect_url}/40.5/src/shell-blur-effect.c
run wget -nc ${blur_effect_url}/40.5/src/shell-blur-effect.h

# patch the code
run cp shell*.[ch] mutter-40.5/src
run cp "$aur"/*.[ch] mutter-40.5/src
run cd mutter-40.5/
run patch -p1 < "${aur}"/rounded_corners.41.1.patch
run patch -p1 < "${aur}"/shell_blur_effect.patch
run patch -p1 < "${dir}"/symbols.patch
run patch -p1 < "${dir}"/colors.diff

# commit changes
run dpkg-source --commit . rounded_corners

# build packages
run _ignore_ debuild

run cd ../
run mv *.deb ../

echo $(green "build finish, you can find packages at: $dir")
