EXAMPLE_DIR = $(datadir)/doc/libxml2-python-$(LIBXML_VERSION)/examples

PYTESTS=	\
    build.py	\
    attribs.py	\
    tst.py	\
    tstxpath.py	\
    xpathext.py	\
    push.py	\
    pushSAX.py	\
    pushSAXhtml.py \
    error.py	\
    serialize.py\
    validate.py	\
    tstURI.py	\
    cutnpaste.py\
    xpathret.py	\
    xpath.py	\
    outbuf.py	\
    inbuf.py	\
    resolver.py \
    regexp.py	\
    reader.py	\
    reader2.py	\
    reader3.py	\
    reader4.py	\
    reader5.py	\
    reader6.py	\
    reader7.py	\
    reader8.py	\
    readernext.py	\
    walker.py	\
    nsdel.py	\
    ctxterror.py\
    readererr.py\
    relaxng.py	\
    schema.py	\
    thread2.py \
    sync.py \
    tstLastError.py \
    indexes.py \
    dtdvalid.py \
    tstmem.py	\
    validDTD.py	\
    validSchemas.py \
    validRNG.py

XMLS=		\
    tst.xml	\
    valid.xml	\
    invalid.xml	\
    test.dtd

EXTRA_DIST = $(PYTESTS) $(XMLS)

if WITH_PYTHON
tests: $(PYTESTS)
	@echo "## running Python regression tests"
	-@(PYTHONPATH="..:../.libs:$(srcdir)/..:$$PYTHONPATH" ; \
	   export PYTHONPATH; \
	   LD_LIBRARY_PATH="$(top_builddir)/.libs:$$LD_LIBRARY_PATH" ; \
	   export LD_LIBRARY_PATH; \
	   for test in $(PYTESTS) ; \
	   do log=`$(PYTHON) $(srcdir)/$$test` ; \
	   if [ "`echo $$log | grep OK`" = "" ] ; then \
	   echo "-- $$test" ; echo "$$log" ; fi ; done)
else
tests:
endif

clean:
	rm -f *.pyc core

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(EXAMPLE_DIR)
	-(for test in $(PYTESTS) $(XMLS); \
	  do @INSTALL@ -m 0644 $(srcdir)/$$test $(DESTDIR)$(EXAMPLE_DIR) ; done)

