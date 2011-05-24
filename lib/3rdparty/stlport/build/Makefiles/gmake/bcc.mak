# Time-stamp: <07/05/31 01:03:15 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Copyright (c) 2006, 2007
# Francois Dumont
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ALL_TAGS = all-static all-shared
ifdef LIBNAME
INSTALL_TAGS = install-static install-shared
else
INSTALL_TAGS = install-shared
endif

ifneq ($(OSNAME),linux)

# For Borland Cygwin/MSys are only build environment, they do not represent
# the targetted OS so per default we keep all generated files in STLport
# folder.
BASE_INSTALL_DIR ?= ${STLPORT_DIR}

CXX := bcc32 
CC := bcc32
RC := brcc32

DEFS ?=
OPT ?=

CFLAGS = -q -ff 
CXXFLAGS = -q -ff 

OPT += -w-ccc -w-rch -w-ngu -w-inl -w-eff

# release-shared : OPT += -w-inl

ifdef WITH_DYNAMIC_RTL
release-static : OPT += -tWR
dbg-static : OPT += -tWR
stldbg-static : OPT += -tWR
endif

ifndef WITH_STATIC_RTL
release-shared : OPT += -tWR
dbg-shared : OPT += -tWR
stldbg-shared : OPT += -tWR
endif

ifdef WITHOUT_RTTI
OPT += -RT-
endif

ifndef WITHOUT_THREAD
OPT += -tWM
endif

WINVER ?= 0x0501
DEFS += -DWINVER=$(WINVER)

OUTPUT_OPTION = -o$@
LINK_OUTPUT_OPTION = $@
CPPFLAGS = $(DEFS) $(OPT) $(INCLUDES) 

CDEPFLAGS = -E -M
CCDEPFLAGS = -E -M
RCFLAGS = -32 -r -i${STLPORT_INCLUDE_DIR} -dCOMP=bcc

release-shared : RCFLAGS += -dBUILD_INFOS="-O2 -vi-"
dbg-shared : RCFLAGS += -dBUILD=d -dBUILD_INFOS="-R -v -y -D_DEBUG"
stldbg-shared : RCFLAGS += -dBUILD=stld -dBUILD_INFOS="-R -v -y -D_DEBUG -D_STLP_DEBUG"
RC_OUTPUT_OPTION = -fo$@

COMPILE.rc = ${RC} ${RCFLAGS}
LINK.cc = ilink32 $(subst /,\\,$(LDFLAGS))

LDFLAGS += -ap -D -Gn

dbg-static : DEFS += -D_DEBUG
dbg-shared : DEFS += -D_DEBUG
stldbg-static : DEFS += -D_DEBUG
stldbg-shared : DEFS += -D_DEBUG

# STLport DEBUG mode specific defines
stldbg-static :	    DEFS += -D_STLP_DEBUG
stldbg-shared :     DEFS += -D_STLP_DEBUG
stldbg-static-dep : DEFS += -D_STLP_DEBUG
stldbg-shared-dep : DEFS += -D_STLP_DEBUG

# optimization and debug compiler flags
release-static : OPT += -O2 -vi-
release-shared : OPT += -O2 -vi-

LDLIBS += import32.lib kernel32.lib
ifndef WITHOUT_THREAD
ifndef WITH_STATIC_RTL
release-shared : LDLIBS += cw32mti.lib
dbg-shared : LDLIBS += cw32mti.lib
stldbg-shared : LDLIBS += cw32mti.lib
else
release-shared : LDLIBS += cw32mt.lib
dbg-shared : LDLIBS += cw32mt.lib
stldbg-shared : LDLIBS += cw32mt.lib
endif
ifndef WITH_DYNAMIC_RTL
release-static : LDLIBS += cw32mt.lib
dbg-static : LDLIBS += cw32mt.lib
stldbg-static : LDLIBS += cw32mt.lib
else
release-static : LDLIBS += cw32mti.lib
dbg-static : LDLIBS += cw32mti.lib
stldbg-static : LDLIBS += cw32mti.lib
endif
else
ifndef WITH_STATIC_RTL
release-shared : LDLIBS += cw32i.lib
dbg-shared : LDLIBS += cw32i.lib
stldbg-shared : LDLIBS += cw32i.lib
else
release-shared : LDLIBS += cw32.lib
dbg-shared : LDLIBS += cw32.lib
stldbg-shared : LDLIBS += cw32.lib
endif
ifndef WITH_DYNAMIC_RTL
release-static : LDLIBS += cw32.lib
dbg-static : LDLIBS += cw32.lib
stldbg-static : LDLIBS += cw32.lib
else
release-static : LDLIBS += cw32i.lib
dbg-static : LDLIBS += cw32i.lib
stldbg-static : LDLIBS += cw32i.lib
endif
endif

# map output option (see build/Makefiles/gmake/dmc.mak)

MAP_OUTPUT_OPTION = 

else # linux

CXX := bc++
CC := bc++

DEFS ?=
OPT ?=

CFLAGS = -q -ff -xp -w-par
CXXFLAGS = -q -ff -xp -w-aus

DEFS += -D_NO_VCL

release-shared: DEFS += -D_RTLDLL
dbg-shared:  DEFS += -D_RTLDLL
stldbg-shared:  DEFS += -D_RTLDLL

OPT += -w-ccc -w-rch -w-ngu -w-inl -w-eff

ifdef WITHOUT_RTTI
OPT += -RT-
endif

ifndef WITHOUT_THREAD
DEFS += -D__MT__
endif

OUTPUT_OPTION = -o$@
LINK_OUTPUT_OPTION = $@
CPPFLAGS = $(DEFS) $(OPT) $(INCLUDES)

LINK.cc = ilink $(LDFLAGS)

LDFLAGS += -Gn 

dbg-static : DEFS += -D_DEBUG
dbg-shared : DEFS += -D_DEBUG
stldbg-static : DEFS += -D_DEBUG
stldbg-shared : DEFS += -D_DEBUG

# STLport DEBUG mode specific defines
stldbg-static :     DEFS += -D_STLP_DEBUG
stldbg-shared :     DEFS += -D_STLP_DEBUG
stldbg-static-dep : DEFS += -D_STLP_DEBUG
stldbg-shared-dep : DEFS += -D_STLP_DEBUG

# optimization and debug compiler flags
release-static : OPT += -O2 -vi-
release-shared : OPT += -O2 -vi-

dbg-static : OPT += -R -v -y
dbg-shared : OPT += -R -v -y
stldbg-static : OPT += -R -v -y
stldbg-shared : OPT += -R -v -y

ifndef WITHOUT_THREAD

ifdef LIBNAME
release-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a
dbg-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a
stldbg-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a
endif

ifdef PRGNAME
release-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so ../../../lib/libstlport.so
dbg-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so ../../../lib/libstlportg.so
stldbg-shared : LDLIBS += libborcrtl.so libborunwind.so libpthread.so.0 libc.so.6 libm.so libdl.so ../../../lib/libstlportstlg.so
release-static : LDLIBS += libborcrtl.a libborunwind.a libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlport.a
dbg-static : LDLIBS += libborcrtl.a libborunwind.a libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlportg.a
stldbg-static : LDLIBS += libborcrtl.a libborunwind.a libpthread.so.0 libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlportstlg.a
endif

else # single-threaded

ifdef LIBNAME
release-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so libc_nonshared.a
dbg-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so libc_nonshared.a
stldbg-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so libc_nonshared.a
endif

ifdef PRGNAME
release-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so ../../../lib/libstlport.so
dbg-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so ../../../lib/libstlportg.so
stldbg-shared : LDLIBS += libborcrtl.so libborunwind.so libc.so.6 libm.so libdl.so ../../../lib/libstlportstlg.so
release-static : LDLIBS += libborcrtl.a libborunwind.a libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlport.a
dbg-static : LDLIBS += libborcrtl.a libborunwind.a libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlportg.a
stldbg-static : LDLIBS += libborcrtl.a libborunwind.a libc.so.6 libm.so libdl.so libc_nonshared.a ../../../lib/libstlportst$
endif

endif

# install dir defaults to /usr/local unless defined

BASE_INSTALL_DIR ?= ${SRCROOT}/..

endif # linux

ifdef EXTRA_CXXFLAGS
CXXFLAGS += $(EXTRA_CXXFLAGS)
endif

ifdef EXTRA_CFLAGS
CFLAGS += $(EXTRA_CFLAGS)
endif

# dependency output parser (dependencies collector)
DP_OUTPUT_DIR = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_DBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_DBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

DP_OUTPUT_DIR_STLDBG = | sed 's|\($*\)\.o[ :]*|$(OUTPUT_DIR_STLDBG)/\1.o $@ : |g' > $@; \
                           [ -s $@ ] || rm -f $@

