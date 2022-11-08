#!/bin/sh
PROJECT_NAME=$1

make CONFIG_CHIP_NAME=BL808 CPU_ID=D0 -j16 PROJECT_NAME=$PROJECT_NAME
cp build_out/$PROJECT_NAME.bin build_out/d0fw.bin
exit $?
