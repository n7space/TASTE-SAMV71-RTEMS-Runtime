#!/bin/bash

PREFIX="${HOME}/tool-inst"
SOURCES=$(dirname $0)

mkdir -p "${PREFIX}/include/TASTE-SAMV71-RTEMS-Runtime/src"
cp -r "${SOURCES}/src" "${PREFIX}/include/TASTE-SAMV71-RTEMS-Runtime"
cp -r "${SOURCES}/arm-bsp" "${PREFIX}/include/TASTE-SAMV71-RTEMS-Runtime"
