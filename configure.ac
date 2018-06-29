dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([DragonFly], [1.0.0], [ninkoman@gmail.com])

dnl Library version information
#AC_SUBST([HELLO_API_VERSION], [0.1])
#AC_SUBST([HELLO_SO_VERSION], [0:0:0])

#AC_CANONICAL_SYSTEM
# for cross compilation
#
AC_CANONICAL_HOST

# inject git revision
#AC_REVISION([m4_esyscmd_s([git describe --always --abbrev=14])])

AC_CONFIG_AUX_DIR([.])

AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

: ${CFLAGS=""}

AC_PROG_CC

LT_INIT
AC_SUBST([LIBTOOL_DEPS])
AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CXX([g++])
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LIBTOOL

AC_LANG([C++])

AC_C_INLINE
AC_TYPE_PID_T

AC_CHECK_HEADERS([stdio.h])

AC_TYPE_SIZE_T

AC_SUBST([AM_CXXFLAGS], [-Wall]) # C++
AC_SUBST([AM_LDFLAGS], [-Wall]) # ld (Linker)

AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([gettimeofday memchr memmove memset strerror strstr strtol strtoul sprintf printf])

AC_CONFIG_FILES(Makefile src/Makefile src/libslice/Makefile src/libslice/examples/Makefile)
AC_OUTPUT

#PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([OPENSSL], [openssl])

