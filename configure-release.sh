#!/bin/bash

source ${0%*.sh}.conf 2> /dev/null

top_dir=$(pwd)

PREFIX=${PREFIX:-$HOME}
LIBDIR=$PREFIX/lib

export PKG_CONFIG_PATH="$LIBDIR/pkgconfig"

export CXXFLAGS="-g0 -O3 -D NDEBUG"

rm -fr $top_dir/build-release
mkdir -p $top_dir/build-release

cd $top_dir/build-release
$top_dir/configure --prefix=$PREFIX --enable-silent-rules



