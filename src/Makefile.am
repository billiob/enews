MAINTAINERCLEANFILES = Makefile.in

bin_PROGRAMS = enews

enews_SOURCES =     \
	enews.c     \
	core.c      \
	dashboard.c

AM_CFLAGS = \
	-I$(top_srcdir)/src/lib \
	-I$(top_builddir)/src/lib \
	-DPACKAGE_LIB_DIR=\"$(libdir)\" \
	-DPACKAGE_BIN_DIR=\"$(bindir)\" \
	-DPACKAGE_DATA_DIR=\"$(datadir)\"\
	@ENEWS_CFLAGS@

LDADD = \
	@ENEWS_LIBS@

clean-local:
	rm -rf *~
