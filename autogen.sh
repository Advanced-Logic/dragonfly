#!/bin/sh

# GNU autotools toolchain
libtoolize
aclocal
autoheader
automake --add-missing --gnu
autoconf
