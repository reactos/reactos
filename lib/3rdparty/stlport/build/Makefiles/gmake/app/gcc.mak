# -*- Makefile -*- Time-stamp: <08/06/12 16:03:31 ptr>
#
# Copyright (c) 1997-1999, 2002, 2003, 2005-2007
# Petr Ovtchenkov
#
# Portion Copyright (c) 1999-2001
# Parallel Graphics Ltd.
#
# Licensed under the Academic Free License version 3.0
#

ifndef NOT_USE_NOSTDLIB

ifeq ($(CXX_VERSION_MAJOR),2)
# i.e. gcc before 3.x.x: 2.95, etc.
# gcc before 3.x don't had libsupc++.a and libgcc_s.so
# exceptions and operators new are in libgcc.a
#  Unfortunatly gcc before 3.x has a buggy C++ language support outside stdc++, so definition of STDLIB below is commented
NOT_USE_NOSTDLIB := 1
#STDLIBS := $(shell ${CXX} -print-file-name=libgcc.a) -lpthread -lc -lm
endif

ifeq ($(CXX_VERSION_MAJOR),3)
# gcc before 3.3 (i.e. 3.0.x, 3.1.x, 3.2.x) has buggy libsupc++, so we should link with libstdc++ to avoid one
ifeq ($(CXX_VERSION_MINOR),0)
NOT_USE_NOSTDLIB := 1
endif
ifeq ($(CXX_VERSION_MINOR),1)
NOT_USE_NOSTDLIB := 1
endif
ifeq ($(CXX_VERSION_MINOR),2)
NOT_USE_NOSTDLIB := 1
endif
endif

endif

ifndef NOT_USE_NOSTDLIB
ifeq ($(OSNAME),linux)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),openbsd)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),freebsd)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),netbsd)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),sunos)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),darwin)
_USE_NOSTDLIB := 1
endif

ifeq ($(OSNAME),cygming)
_USE_NOSTDLIB := 1
endif
endif

ifndef WITHOUT_STLPORT
ifeq (${STLPORT_LIB_DIR},)
ifneq ($(OSNAME),cygming)
release-shared:	STLPORT_LIB = -lstlport
release-static:	STLPORT_LIB = -Wl,-Bstatic -lstlport -Wl,-Bdynamic
dbg-shared:	STLPORT_LIB = -lstlportg
dbg-static:	STLPORT_LIB = -Wl,-Bstatic -lstlportg -Wl,-Bdynamic
stldbg-shared:	STLPORT_LIB = -lstlportstlg
stldbg-static:	STLPORT_LIB = -Wl,-Bstatic -lstlportstlg -Wl,-Bdynamic
else
LIB_VERSION = ${LIBMAJOR}.${LIBMINOR}
release-shared : STLPORT_LIB = -lstlport.${LIB_VERSION}
dbg-shared     : STLPORT_LIB = -lstlportg.${LIB_VERSION}
stldbg-shared  : STLPORT_LIB = -lstlportstlg.${LIB_VERSION}
endif
else
# STLPORT_LIB_DIR not empty
ifneq ($(OSNAME),cygming)
release-shared:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -lstlport
release-static:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -Wl,-Bstatic -lstlport -Wl,-Bdynamic
dbg-shared:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -lstlportg
dbg-static:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -Wl,-Bstatic -lstlportg -Wl,-Bdynamic
stldbg-shared:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -lstlportstlg
stldbg-static:	STLPORT_LIB = -L${STLPORT_LIB_DIR} -Wl,-Bstatic -lstlportstlg -Wl,-Bdynamic
else
LIB_VERSION = ${LIBMAJOR}.${LIBMINOR}
release-shared : STLPORT_LIB = -L${BASE_INSTALL_DIR}/lib -lstlport.${LIB_VERSION}
dbg-shared     : STLPORT_LIB = -L${BASE_INSTALL_DIR}/lib -lstlportg.${LIB_VERSION}
stldbg-shared  : STLPORT_LIB = -L${BASE_INSTALL_DIR}/lib -lstlportstlg.${LIB_VERSION}
endif
endif

endif

ifdef _USE_NOSTDLIB

# Check whether gcc builded with --disable-shared
ifeq ($(shell ${CXX} ${CXXFLAGS} -print-file-name=libgcc_eh.a),libgcc_eh.a)
# gcc builded with --disable-shared, (no library libgcc_eh.a); all exception support in libgcc.a
_LGCC_EH :=
_LGCC_S := -lgcc
else
# gcc builded with --enable-shared (default)
ifdef USE_STATIC_LIBGCC
# if force usage of static libgcc, then exceptions support should be taken from libgcc_eh
_LGCC_EH := -lgcc_eh
_LGCC_S := -lgcc
else
# otherwise, exceptions support is in libgcc_s.so
_LGCC_EH :=
ifneq ($(OSNAME),darwin)
_LGCC_S := -lgcc_s
else
ifdef GCC_APPLE_CC
ifeq ($(MACOSX_TEN_FIVE),true)
_LGCC_S := -lgcc_s.10.5
else
_LGCC_S := -lgcc_s.10.4
endif
else
_LGCC_S := -lgcc_s
# end of GCC_APPLE_CC
endif
# end of Darwin
endif
# end of !USE_STATIC_LIBGCC
endif
# end of present libgcc_eh.a
endif

# ifeq ($(CXX_VERSION_MAJOR),3)
ifeq ($(OSNAME),linux)
START_OBJ := $(shell for o in crt1.o crti.o crtbegin.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
END_OBJ := $(shell for o in crtend.o crtn.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lpthread -lc -lm
endif

ifeq ($(OSNAME),openbsd)
START_OBJ := $(shell for o in crt0.o crtbegin.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
END_OBJ := $(shell for o in crtend.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lpthread -lc -lm
endif

ifeq ($(OSNAME),freebsd)
# FreeBSD < 5.3 should use -lc_r, while FreeBSD >= 5.3 use -lpthread
PTHR := $(shell if [ ${OSREL_MAJOR} -gt 5 ] ; then echo "pthread" ; else if [ ${OSREL_MAJOR} -lt 5 ] ; then echo "c_r" ; else if [ ${OSREL_MINOR} -lt 3 ] ; then echo "c_r" ; else echo "pthread"; fi ; fi ; fi)
START_OBJ := $(shell for o in crt1.o crti.o crtbegin.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
END_OBJ := $(shell for o in crtend.o crtn.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -l${PTHR} -lc -lm
endif

ifeq ($(OSNAME),netbsd)
START_OBJ := $(shell for o in crt1.o crti.o crtbegin.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
END_OBJ := $(shell for o in crtend.o crtn.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lpthread -lc -lm
endif

ifeq ($(OSNAME),sunos)
START_OBJ := $(shell for o in crt1.o crti.o crtbegin.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
END_OBJ := $(shell for o in crtend.o crtn.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lpthread -lc -lm
endif

ifeq ($(OSNAME),darwin)
# sometimes crt3.o will required: it has __cxa_at_exit, but the same defined in libc.dyn
# at least in Mac OS X 10.4.10 (8R2218)
ifeq ($(CXX_VERSION_MAJOR),3)
# i.e. gcc 3.3
START_OBJ := $(shell for o in crt1.o crt2.o; do ${CXX} ${CXXFLAGS} -print-file-name=$$o; done)
else
START_OBJ := -lcrt1.o
endif
END_OBJ :=
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lpthread -lc -lm -lsupc++ ${_LGCC_EH}
#LDFLAGS += -dynamic
endif

ifeq ($(OSNAME),cygming)
LDFLAGS += -nodefaultlibs
ifndef USE_STATIC_LIBGCC
ifeq ($(shell ${CXX} ${CXXFLAGS} -print-file-name=libgcc_s.a),libgcc_s.a)
_LGCC_S := -lgcc
else
_LGCC_S := -lgcc_s
endif
else
_LGCC_S := -lgcc
endif
ifeq ($(OSREALNAME),mingw)
STDLIBS = ${STLPORT_LIB} -lsupc++ ${_LGCC_S} -lmingw32 -lmingwex -lmsvcrt -lm -lmoldname -lcoldname -lkernel32
else
LDFLAGS += -Wl,-enable-auto-import
ifneq (,$(findstring no-cygwin,$(EXTRA_CXXFLAGS)))
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lmingw32 -lmingwex -lmsvcrt -lm -lmoldname -lcoldname -lkernel32
else
STDLIBS = ${STLPORT_LIB} ${_LGCC_S} -lm -lc -lpthread -lkernel32
endif
endif
else
LDFLAGS += -nostdlib
endif

# endif
# _USE_NOSTDLIB
else
ifndef USE_STATIC_LIBGCC
release-shared : LDFLAGS += -shared-libgcc
dbg-shared : LDFLAGS += -shared-libgcc
stldbg-shared : LDFLAGS += -shared-libgcc
endif
ifndef WITHOUT_STLPORT
STDLIBS = ${STLPORT_LIB}
else
STDLIBS = 
endif
endif

# workaround for gcc 2.95.x bug:
ifeq ($(CXX_VERSION_MAJOR),2)
ifneq ($(OSNAME),cygming)
OPT += -fPIC
endif
endif
