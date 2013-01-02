# -*- Makefile -*- Time-stamp: <08/06/12 16:41:21 ptr>

SRCROOT := ../..
COMPILER_NAME := icc
-include ${SRCROOT}/Makefiles/gmake/config.mak
ALL_TAGS = release-shared check-release
CHECK_TAGS = check-release
ifndef WITHOUT_STLPORT
ALL_TAGS += stldbg-shared check-stldbg
CHECK_TAGS += check-stldbg
endif
STLPORT_DIR ?= ../../..

include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

ifdef WITHOUT_STLPORT
DEFS += -DWITHOUT_STLPORT
endif

dbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED
ifndef WITHOUT_STLPORT
stldbg-shared:	DEFS += -D_STLP_DEBUG_UNINITIALIZED
endif

ifdef STLP_BUILD_BOOST_PATH
INCLUDES += -I${STLP_BUILD_BOOST_PATH}
endif

ifndef WITHOUT_STLPORT
release-shared: LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}

ifeq ($(OSNAME),linux)
ifeq ($(CXX_VERSION_MAJOR),8)
ifeq ($(CXX_VERSION_MINOR),0)
# 8.0 build 20031016Z
release-shared:	LDLIBS = -lpthread -lstlport
stldbg-shared:	LDLIBS = -lpthread -lstlportstlg
dbg-shared:	LDLIBS = -lpthread -lstlportg
else
# 8.1 build 028
release-shared:	LDLIBS = -lpthread -lstlport -lcprts -lunwind
stldbg-shared:	LDLIBS = -lpthread -lstlportstlg -lcprts -lunwind
dbg-shared:	LDLIBS = -lpthread -lstlportg -lcprts -lunwind
endif
else
ifeq ($(CXX_VERSION_MAJOR),9)
# 9.0 build 20050430
release-shared:	LDLIBS = -lpthread -lstlport -lcprts -lunwind
stldbg-shared:	LDLIBS = -lpthread -lstlportstlg -lcprts -lunwind
dbg-shared:	LDLIBS = -lpthread -lstlportg -lcprts -lunwind
else
# 7.1 build 20030307Z
release-shared:	LDLIBS = -lpthread -lstlport
stldbg-shared:	LDLIBS = -lpthread -lstlportstlg
dbg-shared:	LDLIBS = -lpthread -lstlportg
endif
endif
endif

endif

check-release:	release-shared
	-${OUTPUT_DIR}/${PRGNAME}

ifndef WITHOUT_STLPORT
check-stldbg:	stldbg-shared
	-${OUTPUT_DIR_STLDBG}/${PRGNAME}
endif

check:	${CHECK_TAGS}

