AUTOMAKE_OPTIONS     = 1.4 foreign
MAINTAINERCLEANFILES = Makefile.in

EDJE_FLAGS = -v -id $(top_srcdir)/data/images

filesdir = $(datadir)
files_DATA = enews.edj

EXTRA_DIST = \
	     enews.edc

enews.edj:  enews.edc
	$(edje_cc) $(EDJE_FLAGS) $< $@

clean-local:
	 rm -rf enews.edj

uninstall:
	 rm -rf $(DESTDIR)$(datadir)
