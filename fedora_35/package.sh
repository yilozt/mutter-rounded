#!/bin/bash

# The source of blur effect. You can replace with:
#   - https://gitee.com/mirrors_GNOME/gnome-shell/raw
#   - https://gitlab.gnome.org/GNOME/gnome-shell/-/raw
blur_effect_url="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw"

# 0. Prepare
# Get the absolute path where this script locate
dir="$(cd $(dirname $0); pwd)"
aur="${dir}/../aur"
tool="$dir/../tool"
SUDO=sudo
if [ "$(whoami)" = "root" ]; then
  SUDO=""
fi

topdir=~/rpmbuild

. $tool
cd "${dir}"
mkdir -p workspace
cd workspace

export LANG=en_US.UTF-8

# 1. Download the source rpm
run rm -rf mutter*
run ${SUDO} dnf download mutter --source
run ${SUDO} dnf builddep mutter
run ${SUDO} dnf install fedora-packager
run rpmdev-setuptree
run rpm -ivh mutter*.rpm

# Query version of packages
pkgver=$(rpm -q ./mutter*rpm|cut -d '-' -f 2)

# 2. generate patch for build
run tar -xvf ${topdir}/SOURCES/mutter-${pkgver}.tar.xz
run wget -nc ${blur_effect_url}/${pkgver}/src/shell-blur-effect.c
run wget -nc ${blur_effect_url}/${pkgver}/src/shell-blur-effect.h

run cd mutter-${pkgver}
run git init
run git config user.name "your name"
run git config user.email "email@example.com"
run git add *
run git commit -m 'init'

# 3. Copy the source file from aur and apply the patches
#    Then use `git diff` to generate a big patch for building the package
run cp ../*.[ch] ./src
run cp "${aur}"/*.[ch] ./src
run patch -p1 < "${aur}"/rounded_corners_40.5.patch
run patch -p1 < "${aur}"/shell_blur_effect_40.5.patch
run git add **.[ch]
run git add **.in
run git add src/meson.build
run git diff --cached > ../0001-mutter-rounded-41.1.patch
run cd ..

# 4. apply the patches
run cp ./0001-mutter-rounded-41.1.patch "${topdir}/SOURCES/"
patch_counts=$(grep Patch ${topdir}/SPECS/mutter.spec | wc -l)
to_match="Patch$((patch_counts - 1))"
to_insert="# Mutter rounded patch for 41.1\nPatch${patch_counts}: 0001-mutter-rounded-41.1.patch"
run sed -i \
  "/^${to_match}.*/a ${to_insert}" \
  ${topdir}/SPECS/mutter.spec

# 5. build packages
run rpmbuild -ba "${topdir}/SPECS/mutter.spec"

echo $(green "build finish, you can find packages at:$topdir/RPMS/x86_64" )

