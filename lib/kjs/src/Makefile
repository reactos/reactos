# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.3 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997, 1998 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

#
# Automakefile for JavaScript virtual machine.
# Copyright (c) 1998 New Generation Software (NGS) Oy
#
# Author: Markku Rossi <mtr@ngs.fi>
#

#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
# MA 02111-1307, USA
#


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local/js-0.2.5
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DISTDIR =

pkgdatadir = $(datadir)/js
pkglibdir = $(libdir)/js
pkgincludedir = $(includedir)/js

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux-gnu
host_triplet = i686-pc-linux-gnu
ACLOCAL_FLAGS_FOR_LIBTOOL = 
CC = gcc
CPP = gcc -E
EXTENSIONS =  dl_open.lo xjs.lo xmd5.lo md5c.lo
EXTENSIONS_LIBS =  -ldl
INTERPRETER_FEATURES =  r_std.lo
LD = /usr/bin/ld
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
MAKEINFO = makeinfo
NM = /usr/bin/nm -B
PACKAGE = js
PGCC_BY_PROVENZANO = 
RANLIB = ranlib
U = 
VERSION = 0.2.5
XLC_R_AIX = 

SUBDIRS = tests

# CFLAGS = -g -O2 -Wall -Wno-unused

lib_LTLIBRARIES = libjs.la
include_HEADERS = js.h jsint.h

libjs_la_SOURCES = bc.c heap.c object.c debug.c compiler.c js.c vm.c \
		vmswitch.c vmjumps.c alloc.c utils.c mrgsort.c \
		regex.c crc32.c iostream.c \
		b_array.c b_bool.c b_func.c b_core.c b_number.c \
		b_object.c b_string.c b_date.c b_dir.c b_file.c b_math.c \
		b_regexp.c b_system.c b_vm.c

libjs_la_LIBADD =  r_std.lo  dl_open.lo xjs.lo xmd5.lo md5c.lo
libjs_la_DEPENDENCIES = $(libjs_la_LIBADD)
EXTRA_libjs_la_SOURCES = vmswt0.c r_std.c r_pthrs.c dl_open.c dl_dummy.c \
	xjs.c xcurses.c xmd5.c md5c.c

man_MANS = js.1

bin_PROGRAMS = js

js_SOURCES = main.c getopt.c getopt1.c

js_LDADD = libjs.la  -ldl -lm

# noinst_PROGRAMS = errtest
# errtest_SOURCES = errtest.c
# errtest_LDADD = libjs.a  -ldl -lm

noinst_HEADERS = c1swt0.h eswt0.h c1switch.h c2switch.h eswitch.h \
	c1jumps.h c2jumps.h ejumps.h rentrant.h \
	getopt.h mrgsort.h regex.h md5.h

EXTRA_DIST = operands.def make-swt0.pl make-switch.pl make-jumps.pl \
	make-op-def.pl make-data.c js.1
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../jsconfig.h
CONFIG_CLEAN_FILES = 
LTLIBRARIES =  $(lib_LTLIBRARIES)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LDFLAGS =  -rdynamic
LIBS = 
libjs_la_LDFLAGS = 
libjs_la_OBJECTS =  bc.lo heap.lo object.lo debug.lo compiler.lo js.lo \
vm.lo vmswitch.lo vmjumps.lo alloc.lo utils.lo mrgsort.lo regex.lo \
crc32.lo iostream.lo b_array.lo b_bool.lo b_func.lo b_core.lo \
b_number.lo b_object.lo b_string.lo b_date.lo b_dir.lo b_file.lo \
b_math.lo b_regexp.lo b_system.lo b_vm.lo
PROGRAMS =  $(bin_PROGRAMS)

js_OBJECTS =  main.o getopt.o getopt1.o
js_DEPENDENCIES =  libjs.la
js_LDFLAGS = 
CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@
man1dir = $(mandir)/man1
MANS = $(man_MANS)

NROFF = nroff
HEADERS =  $(include_HEADERS) $(noinst_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in TODO


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(libjs_la_SOURCES) $(EXTRA_libjs_la_SOURCES) $(js_SOURCES)
OBJECTS = $(libjs_la_OBJECTS) $(js_OBJECTS)

all: all-recursive all-am

.SUFFIXES:
.SUFFIXES: .S .c .lo .o .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps src/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-libLTLIBRARIES:

clean-libLTLIBRARIES:
	-test -z "$(lib_LTLIBRARIES)" || rm -f $(lib_LTLIBRARIES)

distclean-libLTLIBRARIES:

maintainer-clean-libLTLIBRARIES:

install-libLTLIBRARIES: $(lib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	@list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(LIBTOOL)  --mode=install $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/$$p"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_DATA) $$p $(DESTDIR)$(libdir)/$$p; \
	  else :; fi; \
	done

uninstall-libLTLIBRARIES:
	@$(NORMAL_UNINSTALL)
	list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  $(LIBTOOL)  --mode=uninstall rm -f $(DESTDIR)$(libdir)/$$p; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

libjs.la: $(libjs_la_OBJECTS) $(libjs_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libjs_la_LDFLAGS) $(libjs_la_OBJECTS) $(libjs_la_LIBADD) $(LIBS)

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed '$(transform)'`; \
	done

js: $(js_OBJECTS) $(js_DEPENDENCIES)
	@rm -f js
	$(LINK) $(js_LDFLAGS) $(js_OBJECTS) $(js_LDADD) $(LIBS)

install-man1:
	$(mkinstalldirs) $(DESTDIR)$(man1dir)
	@list='$(man1_MANS)'; \
	l2='$(man_MANS)'; for i in $$l2; do \
	  case "$$i" in \
	    *.1*) list="$$list $$i" ;; \
	  esac; \
	done; \
	for i in $$list; do \
	  if test -f $(srcdir)/$$i; then file=$(srcdir)/$$i; \
	  else file=$$i; fi; \
	  ext=`echo $$i | sed -e 's/^.*\\.//'`; \
	  inst=`echo $$i | sed -e 's/\\.[0-9a-z]*$$//'`; \
	  inst=`echo $$inst | sed '$(transform)'`.$$ext; \
	  echo " $(INSTALL_DATA) $$file $(DESTDIR)$(man1dir)/$$inst"; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(man1dir)/$$inst; \
	done

uninstall-man1:
	@list='$(man1_MANS)'; \
	l2='$(man_MANS)'; for i in $$l2; do \
	  case "$$i" in \
	    *.1*) list="$$list $$i" ;; \
	  esac; \
	done; \
	for i in $$list; do \
	  ext=`echo $$i | sed -e 's/^.*\\.//'`; \
	  inst=`echo $$i | sed -e 's/\\.[0-9a-z]*$$//'`; \
	  inst=`echo $$inst | sed '$(transform)'`.$$ext; \
	  echo " rm -f $(DESTDIR)$(man1dir)/$$inst"; \
	  rm -f $(DESTDIR)$(man1dir)/$$inst; \
	done
install-man: $(MANS)
	@$(NORMAL_INSTALL)
	$(MAKE) install-man1
uninstall-man:
	@$(NORMAL_UNINSTALL)
	$(MAKE) uninstall-man1

install-includeHEADERS: $(include_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(includedir)
	@list='$(include_HEADERS)'; for p in $$list; do \
	  if test -f "$$p"; then d= ; else d="$(srcdir)/"; fi; \
	  echo " $(INSTALL_DATA) $$d$$p $(DESTDIR)$(includedir)/$$p"; \
	  $(INSTALL_DATA) $$d$$p $(DESTDIR)$(includedir)/$$p; \
	done

uninstall-includeHEADERS:
	@$(NORMAL_UNINSTALL)
	list='$(include_HEADERS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(includedir)/$$p; \
	done

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.



all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive  \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	rev=''; list='$(SUBDIRS)'; for subdir in $$list; do \
	  rev="$$subdir $$rev"; \
	done; \
	for subdir in $$rev; do \
	  target=`echo $@ | sed s/-recursive//`; \
	  echo "Making $$target in $$subdir"; \
	  (cd $$subdir && $(MAKE) $$target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $(SOURCES) $(HEADERS) $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
	done; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = src

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
	for subdir in $(SUBDIRS); do \
	  test -d $(distdir)/$$subdir \
	  || mkdir $(distdir)/$$subdir \
	  || exit 1; \
	  chmod 777 $(distdir)/$$subdir; \
	  (cd $$subdir && $(MAKE) top_distdir=../$(top_distdir) distdir=../$(distdir)/$$subdir distdir) \
	    || exit 1; \
	done
info: info-recursive
dvi: dvi-recursive
check: all-am
	$(MAKE) check-recursive
installcheck: installcheck-recursive
all-am: Makefile $(LTLIBRARIES) $(PROGRAMS) $(MANS) $(HEADERS)

install-exec-am: install-libLTLIBRARIES install-binPROGRAMS

install-data-am: install-man install-includeHEADERS

uninstall-am: uninstall-libLTLIBRARIES uninstall-binPROGRAMS uninstall-man uninstall-includeHEADERS

install-exec: install-exec-recursive install-exec-am
	@$(NORMAL_INSTALL)

install-data: install-data-recursive install-data-am
	@$(NORMAL_INSTALL)

install: install-recursive install-exec-am install-data-am
	@:

uninstall: uninstall-recursive uninstall-am

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs: installdirs-recursive
	$(mkinstalldirs)  $(DATADIR)$(libdir) $(DATADIR)$(bindir) \
		$(DESTDIR)$(mandir)/man1 $(DATADIR)$(includedir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean-am:  mostlyclean-libLTLIBRARIES mostlyclean-compile \
		mostlyclean-libtool mostlyclean-binPROGRAMS \
		mostlyclean-tags mostlyclean-generic

clean-am:  clean-libLTLIBRARIES clean-compile clean-libtool \
		clean-binPROGRAMS clean-tags clean-generic \
		mostlyclean-am

distclean-am:  distclean-libLTLIBRARIES distclean-compile \
		distclean-libtool distclean-binPROGRAMS distclean-tags \
		distclean-generic clean-am

maintainer-clean-am:  maintainer-clean-libLTLIBRARIES \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-binPROGRAMS maintainer-clean-tags \
		maintainer-clean-generic distclean-am

mostlyclean:  mostlyclean-recursive mostlyclean-am

clean:  clean-recursive clean-am

distclean:  distclean-recursive distclean-am
	-rm -f config.status
	-rm -f libtool

maintainer-clean:  maintainer-clean-recursive maintainer-clean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: mostlyclean-libLTLIBRARIES distclean-libLTLIBRARIES \
clean-libLTLIBRARIES maintainer-clean-libLTLIBRARIES \
uninstall-libLTLIBRARIES install-libLTLIBRARIES mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS install-man1 uninstall-man1 install-man \
uninstall-man uninstall-includeHEADERS install-includeHEADERS \
install-data-recursive uninstall-data-recursive install-exec-recursive \
uninstall-exec-recursive installdirs-recursive uninstalldirs-recursive \
all-recursive check-recursive installcheck-recursive info-recursive \
dvi-recursive mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info dvi \
installcheck all-am install-exec-am install-data-am uninstall-am \
install-exec install-data install uninstall all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


operands:
	./make-op-def.pl operands.def > op.def
	./make-swt0.pl operands.def
	./make-switch.pl operands.def
	./make-jumps.pl operands.def

compiler-source: make-data
	./make-data js_compiler_bytecode ../jsc/compiler.jsc compiler.c

make-data: make-data.o
	$(CC) $(ALLLDFLAGS) make-data.o -o make-data

wc:
	wc alloc.c b_*.c bc.c debug.c dl_*.c heap.c iostream.c js.c	\
	main.c mrgsort.c object.c r_*.c utils.c vm.c vmjumps.c		\
	vmswitch.c vmswt0.c xcurses.c xjs.c xmd5.c js.h jsint.h		\
	mrgsort.h rentrant.h operands.def

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
