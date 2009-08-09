INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libxslt $(LIBXML_CFLAGS)

lib_LTLIBRARIES = libxslt.la

xsltincdir = $(includedir)/libxslt

xsltinc_HEADERS = 			\
	xslt.h				\
	xsltutils.h			\
	pattern.h			\
	templates.h			\
	variables.h			\
	keys.h				\
	numbersInternals.h		\
	extensions.h			\
	extra.h				\
	functions.h			\
	namespaces.h			\
	imports.h			\
	attributes.h			\
	documents.h			\
	preproc.h			\
	transform.h			\
	security.h			\
	xsltInternals.h			\
	xsltconfig.h			\
	xsltexports.h

libxslt_la_SOURCES = 			\
	attrvt.c			\
	xslt.c				\
	xsltutils.c			\
	pattern.c			\
	templates.c			\
	variables.c			\
	keys.c				\
	numbers.c			\
	extensions.c			\
	extra.c				\
	functions.c			\
	namespaces.c			\
	imports.c			\
	attributes.c			\
	documents.c			\
	preproc.c			\
	transform.c			\
	security.c			\
	win32config.h			\
	xsltwin32config.h		\
	xsltwin32config.h.in		\
	libxslt.h

libxslt_la_LIBADD = $(EXTRA_LIBS)
libxslt_la_LDFLAGS = -version-info @LIBXSLT_VERSION_INFO@

man_MANS = libxslt.3

EXTRA_DIST = $(man_MANS) trio.h triodef.h

xsltproc: all
	@(cd ../xsltproc ; $(MAKE))

install-exec-hook:
	$(mkinstalldirs) "$(DESTDIR)$(libdir)/libxslt-plugins"
