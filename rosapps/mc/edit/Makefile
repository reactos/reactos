# Generated automatically from Makefile.in by configure.
srcdir = .

rootdir = $(srcdir)/..
include ../Make.common

CFLAGS = $(XCFLAGS)
CPPFLAGS = $(XCPPFLAGS)
LDFLAGS = $(XLDFLAGS) 
DEFS = $(XDEFS)
LIBS =  $(XLIBS)   $(XLIB)
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
AR = /usr/bin/ar

#
# Distribution variables
#

EDITSRC = edit.c editcmd.c editwidget.c edit_key_translator.c editdraw.c \
	  edit.h editmenu.c editcmddef.h wordproc.c syntax.c editoptions.c

EDITOBJS = edit.o editcmd.o editwidget.o editdraw.o editmenu.o wordproc.o \
	  syntax.o editoptions.o

DIST = 	Makefile.in README.edit $(EDITSRC)

all: libedit.a

.c.o:
	$(CC) -c $(CPPFLAGS) $(DEFS) $(CFLAGS) -DMIDNIGHT $<

check:
	@echo no tests are supplied.

libedit.a: $(EDITOBJS)
	$(RMF) $@
	$(AR) cr $@ $(EDITOBJS)
	-$(RANLIB) $@

mcedit:
	-$(RMF) $(DESTDIR)$(bindir)/$(binprefix)mcedit
	$(LN_S) mc $(DESTDIR)$(bindir)/$(binprefix)mcedit

showlibdep:
	@echo 'OBJS="$(EDITOBJS)"'

cross:
	$(MAKE) CC=gcc-linux CPP="gcc-linux -E" \
	CPPFLAGS="$(CPPFLAGS) -I/usr/local/lib/gcc-lib/i386-linux-linux/include/ncurses "

TAGS: $(EDITSRC)
	etags $(EDITSRC)

clean:
	$(RMF) *.o core a.out libedit.a

realclean: clean
	$(RMF) .depend
	$(RMF) TAGS
	$(RMF) *~

distclean:
	-$(RMF) $(srcdir)/*~ $(srcdir)/*.o $(srcdir)/a.out
	-$(RMF) $(srcdir)/core $(srcdir)/libedit.a
	-if test $(srcdir) = .; then $(MAKE) realclean; fi
	-$(RMF) $(srcdir)/Makefile

install: mcedit

uninstall:
	-$(RMF) $(DESTDIR)$(bindir)/$(binprefix)mcedit

distcopy:
	$(CP) $(DIST) ../../mc-$(VERSION)/edit

depend dep: mcdep

fastdeploc:

# ***Dependencies***Do not edit***
ifeq (.depend,$(wildcard .depend)) 
include .depend 
endif
# ***End of dependencies***
