EXTRADEFS = -D_SETUPAPI_
TOPSRCDIR = ../..
TOPOBJDIR = ../..
SRCDIR    = .

MODULE    = setupapi.dll
IMPORTS   = user32 version advapi32 rpcrt4 kernel32 ntdll
DELAYIMPORTS = shell32
EXTRALIBS = $(LIBUNICODE)

C_SRCS = \
	devinst.c \
	dirid.c \
	diskspace.c \
	install.c \
	misc.c \
	parser.c \
	queue.c \
	setupcab.c \
	stubs.c

C_SRCS16 = \
	devinst16.c \
	infparse.c \
	setupx_main.c \
	virtcopy.c

SPEC_SRCS16 = setupx.spec

RC_SRCS= setupapi.rc


# Global rules for building dlls     -*-Makefile-*-
#
# Each individual makefile should define the following variables:
# MODULE       : name of the main module being built
# EXTRALIBS    : extra libraries to link in (optional)
# SPEC_SRCS16  : interface definition files for 16-bit dlls (optional)
# SUBSYSTEM    : (optional) subsystem (for native dlls)
#
# plus all variables required by the global Make.rules.in
#

DLLDEFS     = 
DLLFLAGS    = -D_REENTRANT
DLLEXT      = 
DEFS        = -D__WINESRC__ $(DLLDEFS) $(EXTRADEFS)
MAINSPEC    = $(MODULE:%.dll=%).spec
SPEC_DEF    = $(MAINSPEC).def
WIN16_FILES = $(SPEC_SRCS16:.spec=.spec.o) $(C_SRCS16:.c=.o) $(EXTRA_OBJS16)
ALL_OBJS    =  $(OBJS) $(MODULE).dbg.o
ALL_LIBS    = $(LIBWINE) $(EXTRALIBS) $(LIBPORT) $(LDFLAGS) $(LIBS)
IMPORTLIBS  = $(DELAYIMPORTS:%=$(DLLDIR)/lib%.$(IMPLIBEXT)) $(IMPORTS:%=$(DLLDIR)/lib%.$(IMPLIBEXT))

all: $(MODULE)$(DLLEXT) $(SUBDIRS)


# Global rules shared by all makefiles     -*-Makefile-*-
#
# Each individual makefile must define the following variables:
# TOPSRCDIR    : top-level source directory
# TOPOBJDIR    : top-level object directory
# SRCDIR       : source directory for this module
# MODULE       : name of the module being built
#
# Each individual makefile may define the following additional variables:
# C_SRCS       : C sources for the module
# C_SRCS16     : 16-bit C sources for the module
# RC_SRCS      : resource source files
# EXTRA_SRCS   : extra source files for make depend
# EXTRA_OBJS   : extra object files
# IMPORTS      : dlls to import
# DELAYIMPORTS : dlls to import in delayed mode
# SUBDIRS      : subdirectories that contain a Makefile
# EXTRASUBDIRS : subdirectories that do not contain a Makefile
# INSTALLSUBDIRS : subdirectories to run make install/uninstall into

# First some useful definitions

SHELL     = /bin/sh
CC        = mingw32-gcc
CFLAGS    = -g -O2
CPPFLAGS  = 
LIBS      = 
BISON     = bison
YACC      = $(BISON) -y
LEX       = flex
LEXLIB    = 
EXEEXT    = .exe
OBJEXT    = o
LIBEXT    = dll
DLLEXT    = 
IMPLIBEXT = a
LDSHARED  = 
DLLTOOL   = mingw32-dlltool
DLLWRAP   = mingw32-dllwrap
AR        = mingw32-ar rc
RANLIB    = mingw32-ranlib
STRIP     = mingw32-strip
WINDRES   = mingw32-windres
LN        = ln
LN_S      = ln -s
TOOLSDIR  = /usr/src/wine-tools/
AS        = mingw32-as
LD        = mingw32-ld
LDFLAGS   = 
RM        = rm -f
MV        = mv
LINT      = 
LINTFLAGS = 
FONTFORGE = fontforge
INCLUDES     = -I$(SRCDIR) -I. -I$(TOPSRCDIR)/include -I$(TOPOBJDIR)/include $(EXTRAINCL)
EXTRACFLAGS  = -Wall -pipe -fno-strength-reduce -mpreferred-stack-boundary=2 -fno-strict-aliasing -gstabs+ -Wpointer-arith
ALLCFLAGS    = $(INCLUDES) $(DEFS) $(DLLFLAGS) $(EXTRACFLAGS) $(CPPFLAGS) $(CFLAGS)
ALLLINTFLAGS = $(INCLUDES) $(DEFS) $(LINTFLAGS)
IDLFLAGS     = $(INCLUDES) $(DEFS) $(EXTRAIDLFLAGS)
MKINSTALLDIRS= $(TOPSRCDIR)/tools/mkinstalldirs -m 755
WINAPI_CHECK = $(TOPSRCDIR)/tools/winapi_check/winapi_check
WINEWRAPPER  = $(TOPSRCDIR)/tools/winewrapper
C2MAN        = $(TOPSRCDIR)/tools/c2man.pl
RUNTEST      = $(TOPSRCDIR)/tools/runtest
WINEBUILD    = $(TOOLSDIR)/tools/winebuild/winebuild
MAKEDEP      = $(TOOLSDIR)/tools/makedep
WRC          = $(TOOLSDIR)/tools/wrc/wrc
BIN2RES      = $(TOOLSDIR)/tools/bin2res
WMC          = $(TOOLSDIR)/tools/wmc/wmc
WIDL         = $(TOOLSDIR)/tools/widl/widl
WINEGCC      = $(TOOLSDIR)/tools/winegcc/winegcc
SFNT2FNT     = $(TOOLSDIR)/tools/sfnt2fnt
FNT2FON      = $(TOOLSDIR)/tools/fnt2fon
RC           = $(WRC)
RC16         = $(WRC)
RCFLAGS      = --nostdinc $(INCLUDES) $(DEFS) $(EXTRARCFLAGS)
RC16FLAGS    = -O res16 $(RCFLAGS)
LDPATH       = LD_LIBRARY_PATH="$(TOOLSDIR)/libs/unicode:$$LD_LIBRARY_PATH"
DLLDIR       = $(TOPOBJDIR)/dlls
LIBDIR       = $(TOPOBJDIR)/libs
LIBPORT      = -L$(TOPOBJDIR)/libs/port -lwine_port
LIBUNICODE   = -L$(TOPOBJDIR)/libs/unicode -lwine_unicode
LIBWINE      = -L$(TOPOBJDIR)/libs/wine -lwine



# Installation infos

INSTALL         = /usr/bin/ginstall -c $(INSTALL_FLAGS)
INSTALL_PROGRAM = ${INSTALL} $(INSTALL_PROGRAM_FLAGS)
INSTALL_SCRIPT  = ${INSTALL} $(INSTALL_SCRIPT_FLAGS)
INSTALL_DATA    = ${INSTALL} -m 644 $(INSTALL_DATA_FLAGS)
prefix          = /usr/local
exec_prefix     = ${prefix}
bindir          = ${exec_prefix}/bin
libdir          = ${exec_prefix}/lib
datadir         = ${prefix}/share
infodir         = ${prefix}/info
mandir          = ${prefix}/man
sysconfdir      = ${prefix}/etc
includedir      = ${prefix}/include/wine
dlldir          = ${exec_prefix}/lib/wine
prog_manext     = 1
api_manext      = 3w
conf_manext     = 5
CLEAN_FILES     = *.o *.a *.so *.ln *.$(LIBEXT) \\\#*\\\# *~ *% .\\\#* *.bak *.orig *.rej \
                  *.flc *.spec.c *.spec.def *.dbg.c *.tab.c *.tab.h lex.yy.c core

OBJS = $(C_SRCS:.c=.o) $(EXTRA_OBJS)

RCOBJS = $(RC_SRCS:.rc=.res.o)
LINTS  = $(C_SRCS:.c=.ln)

# Implicit rules

.SUFFIXES: .mc .rc .mc.rc .res .res.o .spec .spec.c .spec.def .idl .h .ok .sfd .ttf

.c.o:
	$(CC) -c $(ALLCFLAGS) -o $@ $<

.s.o:
	$(AS) -o $@ $<

.mc.mc.rc:
	$(LDPATH) $(WMC) -i -U -H /dev/null -o $@ $<

.rc.res:
	$(LDPATH) $(RC) $(RCFLAGS) -fo$@ $<

.res.res.o:
	$(WINDRES) -i $< -o $@

.spec.spec.c:
	$(WINEBUILD) $(DEFS) -o $@ --main-module $(MODULE) --dll $<

.spec.spec.def:
	$(WINEBUILD) -w $(DEFS) -o $@ --def $<

.idl.h:
	$(WIDL) $(IDLFLAGS) -h -H $@ $<

.c.ln:
	$(LINT) -c $(ALLLINTFLAGS) $< || ( $(RM) $@ && exit 1 )

.c.ok:
	$(RUNTEST) $(RUNTESTFLAGS) $< && touch $@

.sfd.ttf:
	$(FONTFORGE) -script $(TOPSRCDIR)/fonts/genttf.ff $<

# 'all' target first in case the enclosing Makefile didn't define any target

all: Makefile

filter:
	@$(TOPSRCDIR)/tools/winapi/make_filter --make $(MAKE) all

.PHONY: all filter

# Rules for resources

$(RC_BINARIES): $(BIN2RES) $(RC_BINSRC)
	$(BIN2RES) -f -o $@ $(SRCDIR)/$(RC_BINSRC)

$(RC_SRCS:.rc=.res) $(RC_SRCS16:.rc=.res): $(WRC) $(RC_BINARIES)

# Rule for main module debug channels

$(MODULE).dbg.c: $(C_SRCS) $(C_SRCS16) $(WINEBUILD)
	$(WINEBUILD) $(DEFS) -o $@ --debug -C$(SRCDIR) $(C_SRCS) $(C_SRCS16)

# Rules for makefile

Makefile: Makefile.in $(TOPSRCDIR)/configure
	@echo Makefile is older than $?, please rerun $(TOPSRCDIR)/configure
	@exit 1

# Rule for linting

$(MODULE).ln : $(LINTS)
	if test "$(LINTS)" ; \
	then \
		$(LINT) $(ALLLINTFLAGS) -o$(MODULE) $(LINTS) ; \
	        $(MV) llib-l$(MODULE).ln $(MODULE).ln ; \
	else \
		$(LINT) $(ALLLINTFLAGS) -C$(MODULE) /dev/null ; \
	fi

lint:: $(MODULE).ln

# Rules for Windows API checking

winapi_check:: dummy
	$(WINAPI_CHECK) $(WINAPI_CHECK_FLAGS) $(WINAPI_CHECK_EXTRA_FLAGS) .

.PHONY: winapi_check

# Rules for dependencies

$(SUBDIRS:%=%/__depend__): dummy
	cd `dirname $@` && $(MAKE) depend

depend: $(IDL_SRCS:.idl=.h) $(SUBDIRS:%=%/__depend__)
	$(MAKEDEP) $(INCLUDES) -C$(SRCDIR) $(C_SRCS) $(C_SRCS16) $(RC_SRCS) $(RC_SRCS16) $(MC_SRCS) $(IDL_SRCS) $(EXTRA_SRCS)

.PHONY: depend $(SUBDIRS:%=%/__depend__)

# Rules for cleaning

$(SUBDIRS:%=%/__clean__): dummy
	cd `dirname $@` && $(MAKE) clean

$(SUBDIRS:%=%/__testclean__): dummy
	cd `dirname $@` && $(MAKE) testclean

$(EXTRASUBDIRS:%=%/__clean__): dummy
	-cd `dirname $@` && $(RM) $(CLEAN_FILES)

testclean:: $(SUBDIRS:%=%/__testclean__)

clean:: $(SUBDIRS:%=%/__clean__) $(EXTRASUBDIRS:%=%/__clean__)
	$(RM) $(CLEAN_FILES) $(RC_SRCS:.rc=.res) $(RC_SRCS16:.rc=.res) $(MC_SRCS:.mc=.mc.rc) $(IDL_SRCS:.idl=.h) $(PROGRAMS) $(RC_BINARIES)

.PHONY: clean testclean $(SUBDIRS:%=%/__clean__) $(SUBDIRS:%=%/__testclean__) $(EXTRASUBDIRS:%=%/__clean__)

# Rules for installing

$(SUBDIRS:%=%/__install__): dummy
	cd `dirname $@` && $(MAKE) install

$(SUBDIRS:%=%/__install-lib__): dummy
	cd `dirname $@` && $(MAKE) install-lib

$(SUBDIRS:%=%/__install-dev__): dummy
	cd `dirname $@` && $(MAKE) install-dev

$(SUBDIRS:%=%/__uninstall__): dummy
	cd `dirname $@` && $(MAKE) uninstall

install:: $(INSTALLSUBDIRS:%=%/__install__)

uninstall:: $(INSTALLSUBDIRS:%=%/__uninstall__)

.PHONY: install install-lib install-dev uninstall \
	$(SUBDIRS:%=%/__install__) $(SUBDIRS:%=%/__uninstall__) \
	$(SUBDIRS:%=%/__install-lib__) $(SUBDIRS:%=%/__install-dev__)

# Rules for checking that no imports are missing

$(SUBDIRS:%=%/__checklink__): dummy
	@cd `dirname $@` && $(MAKE) checklink

.PHONY: checklink $(SUBDIRS:%=%/__checklink__)

# Rules for testing

$(SUBDIRS:%=%/__test__): dummy
	@cd `dirname $@` && $(MAKE) test

$(SUBDIRS:%=%/__crosstest__): dummy
	@cd `dirname $@` && $(MAKE) crosstest

.PHONY: check test crosstest $(SUBDIRS:%=%/__test__) $(SUBDIRS:%=%/__crosstest__)

# Misc. rules

$(MC_SRCS:.mc=.mc.rc): $(WMC)

$(IDL_SRCS:.idl=.h): $(WIDL)

$(SUBDIRS): dummy
	@cd $@ && $(MAKE)

dummy:

.PHONY: dummy $(SUBDIRS)

# End of global rules

# Rules for .so files

$(MODULE).so: $(MAINSPEC) $(RC_SRCS:.rc=.res) $(ALL_OBJS) $(IMPORTLIBS) Makefile.in
	$(WINEGCC) -B$(TOOLSDIR)/tools/winebuild -shared $(SRCDIR)/$(MAINSPEC) $(ALL_OBJS) $(RC_SRCS:.rc=.res) $(SUBSYSTEM:%=-Wb,--subsystem,%) -o $@ -L$(DLLDIR) $(DELAYIMPORTS:%=-Wb,-d%) $(IMPORTS:%=-l%) $(ALL_LIBS)

# Rules for .dll files

$(MODULE): $(RCOBJS) $(OBJS) $(MODULE).dbg.o $(SPEC_DEF) $(IMPORTLIBS) Makefile.in
	$(DLLWRAP) -k --def $(SPEC_DEF) -o $@ $(RCOBJS) $(OBJS) $(MODULE).dbg.o -L$(DLLDIR) $(DELAYIMPORTS:%=-l%) $(IMPORTS:%=-l%) $(ALL_LIBS)

$(SPEC_DEF): $(WINEBUILD)

# Rules for checking that no imports are missing

.PHONY: checklink16 $(WIN16_FILES:%=__checklink16__%)

$(WIN16_FILES:%=__checklink16__%): checklink16

checklink16:: $(MAINSPEC).o $(OBJS) $(MODULE).dbg.o dummy
	$(CC) -o checklink -Wl,-rpath,$(TOPOBJDIR)/libs $(TOPSRCDIR)/dlls/checklink.c $(MAINSPEC).o $(OBJS) $(MODULE).dbg.o -L$(DLLDIR) $(ALL_LIBS) -lm && $(RM) checklink $(MAINSPEC).c $(MAINSPEC).o

checklink:: $(WIN16_FILES:%=__checklink16__%)

# Rules for testing

check test:: $(SUBDIRS:%=%/__test__)

crosstest:: $(SUBDIRS:%=%/__crosstest__)

# Rule to explicitly generate the .spec.c for debugging

$(MAINSPEC).c: $(MAINSPEC) $(RC_SRCS:.rc=.res) $(ALL_OBJS) $(IMPORTLIBS) $(WINEBUILD)
	$(WINEBUILD) $(DEFS) $(DLLFLAGS) -o $@ --dll $(SRCDIR)/$(MAINSPEC) $(SUBSYSTEM:%=--subsystem %) $(RC_SRCS:.rc=.res) $(ALL_OBJS) -L$(DLLDIR) $(DELAYIMPORTS:%=-d%) $(IMPORTS:%=-l%)

# Rules for auto documentation

man: $(C_SRCS)
	$(C2MAN) -o $(TOPOBJDIR)/documentation/man$(api_manext) -R$(TOPOBJDIR) -S$(api_manext) $(INCLUDES) $(MAINSPEC:%=-w %) $(SPEC_SRCS16:%=-w %) $(C_SRCS) $(C_SRCS16)

doc-html: $(C_SRCS)
	$(C2MAN) -o $(TOPOBJDIR)/documentation/html -R$(TOPSRCDIR) $(INCLUDES) -Th $(MAINSPEC:%=-w %) $(SPEC_SRCS16:%=-w %) $(C_SRCS) $(C_SRCS16)

doc-sgml: $(C_SRCS)
	$(C2MAN) -o $(TOPOBJDIR)/documentation/api-guide -R$(TOPSRCDIR) $(INCLUDES) -Ts $(MAINSPEC:%=-w %) $(SPEC_SRCS16:%=-w %) $(C_SRCS) $(C_SRCS16)

.PHONY: man doc-html doc-sgml

# Rules for installation

EXE_SPECS16 = $(SPEC_SRCS16:.exe.spec=.exe)
DRV_SPECS16 = $(EXE_SPECS16:.drv.spec=.drv)
ALL_SPECS16 = $(DRV_SPECS16:.spec=.dll)

WIN16_INSTALL = $(ALL_SPECS16:%=_install_/%)

.PHONY: install_lib $(ALL_SPECS16:%=_install_/%) $(ALL_SPECS16:%=_uninstall_/%)

$(ALL_SPECS16:%=_install_/%): install_lib
	cd $(dlldir) && $(RM) `basename $@`$(DLLEXT) && $(LN_S) $(MODULE)$(DLLEXT) `basename $@`$(DLLEXT)

$(ALL_SPECS16:%=_uninstall_/%): dummy
	$(RM) $(dlldir)/`basename $@`$(DLLEXT)

install_lib: $(MODULE)$(DLLEXT)
	$(MKINSTALLDIRS) $(dlldir)
	$(INSTALL_PROGRAM) $(MODULE)$(DLLEXT) $(dlldir)/$(MODULE)$(DLLEXT)

install:: install_lib 

uninstall:: $(ALL_SPECS16:%=_uninstall_/%)
	$(RM) $(dlldir)/$(MODULE)$(DLLEXT)

# Misc. rules

$(SPEC_SRCS16:.spec=.spec.c): $(WINEBUILD)

# End of global dll rules

### Dependencies:
