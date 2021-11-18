#!/bin/bash

# The source of blur effect. You can replace with:
#   - https://gitee.com/mirrors_GNOME/gnome-shell/raw
#   - https://gitlab.gnome.org/GNOME/gnome-shell/-/raw
blur_effect_url="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw"

# 0. Prepare
# Get the absolute path where this script locate
dir="$(cd $(dirname $0); pwd)"
aur="${dir}/../aur"
SUDO=sudo
if [ "$(whoami)" = "root" ]; then
  SUDO=""
fi

topdir=~/rpmbuild

cd "${dir}"
mkdir -p workspace
cd workspace

export LANG=en_US.UTF-8

# 1. Download the source rpm
${SUDO} dnf download mutter --source
${SUDO} dnf builddep mutter
${SUDO} dnf install fedora-packager
rpmdev-setuptree
rpm -ivh mutter*.rpm

# 2. generate patch for build
[ -d mutter-41.1 ] && rm -rf mutter-41.1
tar -xvf ${topdir}/SOURCES/mutter-41.1.tar.xz
wget -nc ${blur_effect_url}/41.1/src/shell-blur-effect.c
wget -nc ${blur_effect_url}/41.1/src/shell-blur-effect.h

cd mutter-41.1
git init
git config user.name "your name"
git config user.email "email@example.com"
git add *
git commit -m 'init'

# 3. Copy the source file from aur and apply the patches
#    Then use `git diff` to generate a big patch for building the package
cp ../*.[ch] ./src
cp "${aur}"/*.[ch] ./src
patch -p1 < "${aur}"/rounded_corners_40.5.patch
patch -p1 < "${aur}"/shell_blur_effect_40.5.patch
git add **.[ch]
git add **.in
git add src/meson.build
git diff --cached > ../0001-mutter-rounded-41.1.patch
cd ..

# 4. apply the patches
cp ./0001-mutter-rounded-41.1.patch "${topdir}/SOURCES/"
patch_counts=$(grep Patch ${topdir}/SPECS/mutter.spec | wc -l)
to_match="Patch$((patch_counts - 1))"
to_insert="# Mutter rounded patch for 41.1\nPatch${patch_counts}: 0001-mutter-rounded-41.1.patch"
sed -i \
  "/^${to_match}.*/a ${to_insert}" \
  ${topdir}/SPECS/mutter.spec

# 5. build packages
rpmbuild -ba "${topdir}/SPECS/mutter.spec"
