#!/bin/bash
mkdir -p builddir || exit $?
conan install . --output-folder=builddir --build=missing || exit $?
cd builddir || exit $?
meson setup --wipe --warnlevel 3 --native-file conan_meson_native.ini .. meson-src || exit $?