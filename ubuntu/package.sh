#!/bin/bash

# The source of blur effect. You can replace with:
#   - https://gitee.com/mirrors_GNOME/gnome-shell/raw
#   - https://gitlab.gnome.org/GNOME/gnome-shell/-/raw
blur_effect_url="https://gitlab.gnome.org/GNOME/gnome-shell/-/raw"

dir="$(cd $(dirname $0); pwd)"
patches="${dir}/../patches"
tool="$dir/../tool"

SUDO=sudo
if [ "$(whoami)" = "root" ]; then
  SUDO=""
fi

export LANG=en_US.UTF-8

. $tool

declare -A support_patches
support_patches=(
  [21.10]="rounded_corners.41.1.patch"
  [22.04]="rounded_corners.42.0.patch"
)
current_version=$(lsb_release -rs)
code_name=$(lsb_release -cs)
rounded_patch=${support_patches[$current_version]}

if [[ "$rounded_patch" == "" ]]; then
  echo "Supported version: ${!support_patches[@]}"
  echo "Current version: ${current_version}"
  echo exit.
  exit 1
fi

cd "${dir}"
run mkdir -p workspace
run cd workspace

# enable deb-src source, install dependencie
run ${SUDO} sed -i 's/^# deb-src/deb-src/g' /etc/apt/sources.list
run ${SUDO} apt update
run ${SUDO} apt install -y gnupg pbuilder ubuntu-dev-tools apt-file
run ${SUDO} apt build-dep -y mutter

# download the source code
run rm -rf mutter-* shell-blur-effect*
apt -y --download-only source mutter
dpkg-source -x *.dsc
ver=$(ls mutter-* -d |cut -d '-' -f 2)

run wget ${blur_effect_url}/${ver}/src/shell-blur-effect.c
run wget ${blur_effect_url}/${ver}/src/shell-blur-effect.h

# apply patches
run cp shell*.[ch] mutter-${ver}/src
run cp "$patches"/*.[ch] mutter-${ver}/src
run cd mutter-${ver}/
run patch -p1 < "${patches}"/${rounded_patch}
run patch -p1 < "${patches}"/shell_blur_effect.patch
for i in "${dir}"/"${current_version}"/*; do
  run patch -p1 -i $i
done

# commit changes
run dpkg-source --auto-commit -b .

# build packages
run _ignore_ debuild

run cd ../
run mv *.deb ../

echo $(green "build finish, you can find packages at:$dir" )
