# -*- Makefile -*- Time-stamp: <08/06/06 11:01:34 yeti>

SRCROOT := ../..
COMPILER_NAME := gcc
#NOT_USE_NOSTDLIB := 1
#WITHOUT_STLPORT := 1
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

ifndef TARGET_OS
ifndef WITHOUT_STLPORT

ifeq ($(OSNAME), sunos)
release-shared: LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:     LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:  LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}
endif

ifeq ($(OSNAME), freebsd)
release-shared: LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:     LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:  LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}
endif

ifeq ($(OSNAME), openbsd)
release-shared: LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:     LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:  LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,-R${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}
endif

ifeq ($(OSNAME), linux)
release-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,-rpath=${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,-rpath=${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,-rpath=${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}
endif

ifeq ($(OSNAME), hp-ux)
release-shared: LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR} -Wl,+b${STLPORT_DIR}/build/lib/${OUTPUT_DIR}
dbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG} -Wl,+b${STLPORT_DIR}/build/lib/${OUTPUT_DIR_DBG}
stldbg-shared:	LDFLAGS += -L${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG} -Wl,+b${STLPORT_DIR}/build/lib/${OUTPUT_DIR_STLDBG}
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

