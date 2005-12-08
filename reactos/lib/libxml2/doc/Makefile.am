## Process this file with automake to produce Makefile.in
SUBDIRS=examples

# The top-level SGML file.
DOC_MAIN_XML_FILE=gnome-xml.xml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=..

# A file in win32 depends upon one of the doc files
WIN32_DIR=$(top_srcdir)/win32

PAGES= architecture.html bugs.html contribs.html docs.html DOM.html \
    downloads.html entities.html example.html help.html index.html \
    interface.html intro.html library.html namespaces.html news.html \
    tree.html xmldtd.html XMLinfo.html XSLT.html
APIPAGES=APIconstructors.html APIfiles.html APIfunctions.html \
    APIsymbols.html APIchunk0.html
EXTRA_DIST=xmlcatalog_man.xml tutorial/*.html tutorial/*.c tutorial/*.pdf \
           tutorial/images/*.png tutorial/images/callouts/*.png \
	   API*.html *.1 *.xsl *.html *.gif w3c.png html/*.html \
	   html/*.png libxml2-api.xml index.py search.php \
	   apibuild.py libxml2.xsa xmllint.xml xmlcatalog_man.xml \
	   README.docs


man_MANS = xmllint.1 xmlcatalog.1

all: web $(top_builddir)/NEWS libxml2.xsa $(man_MANS)

api: libxml2-api.xml libxml2-refs.xml $(APIPAGES) $(srcdir)/html/index.html $(WIN32_DIR)/libxml2.def.src ../elfgcchack.h

web: $(PAGES)

../elfgcchack.h: $(srcdir)/elfgcchack.xsl $(srcdir)/libxml2-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the elfgcchack.h header" ; \
	  $(XSLTPROC) --nonet $(srcdir)/elfgcchack.xsl $(srcdir)/libxml2-api.xml > elfgcchack.h ; \
	  if [ "`diff -q elfgcchack.h ../elfgcchack.h`" ] ; then \
	    echo "updating ../elfgcchack.h"; \
	    cp elfgcchack.h ../elfgcchack.h; \
	  fi ; rm -f elfgcchack.h ;  fi );

$(PAGES): xml.html site.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML Web pages from xml.html" ; \
	  $(XSLTPROC) --nonet --html $(top_srcdir)/doc/site.xsl $(top_srcdir)/doc/xml.html > index.html ; fi );
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the HTML Web pages" ; \
	  $(XMLLINT) --nonet --valid --noout $(PAGES) ; fi );

$(top_builddir)/NEWS: $(top_srcdir)/doc/news.xsl $(top_srcdir)/doc/news.html
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) --nonet $(top_srcdir)/doc/news.xsl $(top_srcdir)/doc/news.html > $(top_builddir)/NEWS ; fi );

libxml2.xsa: $(top_srcdir)/doc/xsa.xsl $(top_srcdir)/doc/news.html
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the NEWS file" ; \
	  $(XSLTPROC) --nonet $(top_srcdir)/doc/xsa.xsl $(top_srcdir)/doc/news.html > libxml2.xsa ; fi );

$(APIPAGES): libxml2-api.xml libxml2-refs.xml $(top_srcdir)/doc/site.xsl $(top_srcdir)/doc/api.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML API pages from libxml2-refs.xml" ; \
	  $(XSLTPROC) --nonet --html $(top_srcdir)/doc/api.xsl \
	                     $(top_srcdir)/doc/xml.html ; fi );
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the HTML API pages" ; \
	  $(XMLLINT) --nonet --valid --noout API*.html ; fi );

$(srcdir)/html/index.html: libxml2-api.xml $(srcdir)/newapi.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML pages from the XML API" ; \
	  $(XSLTPROC) --nonet $(srcdir)/newapi.xsl libxml2-api.xml ; fi )
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the resulting XHTML pages" ; \
	  $(XMLLINT) --nonet --valid --noout html/*.html ; fi );

wiki: libxml2-api.xml $(srcdir)/wiki.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
		echo "Rebuilding the wiki HTML pages from the XML API" ; \
		$(XSLTPROC) --nonet $(srcdir)/wiki.xsl libxml2-api.xml; fi )

$(WIN32_DIR)/libxml2.def.src: libxml2-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) -o $(WIN32_DIR)/libxml2.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libxml2-api.xml ; fi )

libxml2-api.xml libxml2-refs.xml: apibuild.py ../include/libxml/*.h ../*.c
	-(./apibuild.py)
	-@(cd .. ; $(MAKE) rebuild_testapi)


xmllint.1: xmllint.xml
	-@($(XSLTPROC) --nonet xmllint.xml)

xmlcatalog.1: xmlcatalog_man.xml
	-@($(XSLTPROC) --nonet xmlcatalog_man.xml)

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean-local
	rm -rf libxml-decl-list.txt libxml-decl.txt 

rebuild: api all

install-data-local: 
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)
	-@INSTALL@ -m 0644 $(srcdir)/xml.html $(srcdir)/encoding.html $(srcdir)/FAQ.html $(srcdir)/structure.gif $(srcdir)/DOM.gif $(srcdir)/smallfootonly.gif $(srcdir)/redhat.gif $(srcdir)/libxml.gif $(srcdir)/w3c.png $(srcdir)/Libxml2-Logo-180x168.gif $(srcdir)/Libxml2-Logo-90x34.gif $(DESTDIR)$(HTML_DIR)
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(HTML_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/*.png $(DESTDIR)$(HTML_DIR)/html
	-@INSTALL@ -m 0644 $(srcdir)/html/index.sgml $(DESTDIR)$(HTML_DIR)/html
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)/tutorial
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)/tutorial/images
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/images/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial/images
	$(mkinstalldirs) $(DESTDIR)$(HTML_DIR)/tutorial/images/callouts
	-@INSTALL@ -m 0644 $(srcdir)/tutorial/images/callouts/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial/images/callouts

.PHONY : html xml templates scan
