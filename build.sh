#!/bin/sh
if [ "$1" = "qserver" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- qserver_defconfig
elif [ "$1" = "dtbs" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- dtbs
elif [ "$1" = "uboot" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-  KCFLAGS=-DFORCE_GIGABIT -j2
elif [ "$1" = "menuconfig" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- menuconfig
elif [ "$1" = "savedefconfig" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- savedefconfig
elif [ "$1" = "clean" ]; then
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- distclean
fi
