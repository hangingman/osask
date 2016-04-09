#!/bin/bash
#
# script for cross compile x86_64-w64-mingw32
#
./bootstrap
./configure --prefix=/usr/x86_64-w64-mingw32           \
--build=x86_64-unknown-linux-gnu                       \
--host=x86_64-w64-mingw32                              \
--target=x86_64-w64-mingw32                            \
--disable-silent-rules
make
