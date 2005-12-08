# Makefile for libxml2 python library
AUTOMAKE_OPTIONS = 1.4 foreign

SUBDIRS= . tests

INCLUDES = \
	-I$(PYTHON_INCLUDES) \
	-I$(top_srcdir)/include	\
	-I$(top_builddir)/include \
	-I$(top_builddir)/$(subdir)

DOCS_DIR = $(datadir)/doc/libxml2-python-$(LIBXML_VERSION)
# libxml2class.txt is generated
DOCS = ${srcdir}/TODO

EXTRA_DIST = 			\
	libxml.c		\
	types.c			\
	setup.py		\
	setup.py.in		\
	generator.py		\
	libxml_wrap.h		\
	libxml.py		\
	drv_libxml2.py		\
	libxml2-python-api.xml	\
	$(DOCS)

libxml2mod_la_LDFLAGS = @CYGWIN_EXTRA_LDFLAGS@ @WIN32_EXTRA_LDFLAGS@ -module -avoid-version -L$(top_builddir)/.libs

if WITH_PYTHON
mylibs = \
	$(top_builddir)/libxml2.la

all-local:	libxml2.py

python_LTLIBRARIES = libxml2mod.la

libxml2mod_la_SOURCES = libxml.c types.c libxml2-py.c
libxml2mod_la_LIBADD = $(mylibs) @CYGWIN_EXTRA_PYTHON_LIBADD@

libxml2.py: $(srcdir)/libxml.py libxml2class.py
	cat $(srcdir)/libxml.py libxml2class.py > libxml2.py

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pythondir)
	@INSTALL@ -m 0644 libxml2.py $(DESTDIR)$(pythondir)
	@INSTALL@ -m 0644 $(srcdir)/drv_libxml2.py $(DESTDIR)$(pythondir)
	$(mkinstalldirs) $(DESTDIR)$(DOCS_DIR)
	@(for doc in $(DOCS) ; \
	   do @INSTALL@ -m 0644 $$doc $(DESTDIR)$(DOCS_DIR) ; done)

GENERATE = generator.py
API_DESC = $(top_srcdir)/doc/libxml2-api.xml $(srcdir)/libxml2-python-api.xml
GENERATED= libxml2class.py \
           libxml2-export.c \
           libxml2class.txt \
	   libxml2-py.c \
	   libxml2-py.h

CLEANFILES= $(GENERATED) gen_prog libxml2.py

$(GENERATED): gen_prog

gen_prog: $(srcdir)/$(GENERATE) $(API_DESC)
	$(PYTHON) $(srcdir)/$(GENERATE) $(srcdir)
	touch gen_prog

$(libxml2mod_la_OBJECTS): $(GENERATED)

else
all: 
endif
tests test: all
	cd tests && $(MAKE) MAKEFLAGS+=--silent tests

