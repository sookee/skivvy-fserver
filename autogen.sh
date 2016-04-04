#! /bin/sh
mkdir -p m4
mkdir -p aux-bits
autoheader --warnings=all
autoreconf --force --install -I aux-bits -I m4
