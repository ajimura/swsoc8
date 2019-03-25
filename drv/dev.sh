#!/bin/sh
mknod /dev/swsoc0 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 0
mknod /dev/swsoc1 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 1
mknod /dev/swsoc2 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 2
mknod /dev/swsoc3 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 3
mknod /dev/swsoc4 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 4
mknod /dev/swsoc5 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 5
mknod /dev/swsoc6 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 6
mknod /dev/swsoc7 c  $(awk "\$2==\"swsoc\" {print \$1}" /proc/devices) 7
chmod 0666 /dev/swsoc*
