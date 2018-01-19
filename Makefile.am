AUTOMAKE_OPTIONS=foreign
SUBDIRS=src src/libslice src/libslice/examples

ACLOCAL_AMFLAGS = -I m4

LIBTOOL_DEPS=@LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool
	