#!/bin/bash

source ${0%*.sh}.conf 2> /dev/null

top_dir=$(pwd)

PREFIX=${PREFIX:-$HOME/dev}
LIBDIR=$PREFIX/lib

export PKG_CONFIG_PATH="$LIBDIR/pkgconfig"

export CXXFLAGS="-g3 -O0 -D DEBUG"

rm -fr $top_dir/build-debug
mkdir -p $top_dir/build-debug

cd $top_dir/build-debug
$top_dir/configure --prefix=$PREFIX



