

# $Id: makefile,v 1.1 2004/05/29 00:15:44 sedwards Exp $

PATH_TO_TOP = ../../..

TARGET_BOOTSTRAP = yes

TARGET_TYPE = library

TARGET_NAME = oskittcp

TARGET_CFLAGS = -g -D__REACTOS__=1 -DKERNEL -Iinclude/freebsd -Iinclude/freebsd/sys/include -Iinclude/freebsd/src/sys -Iinclude/freebsd/dev/include -Iinclude/freebsd/net/include -Iinclude -I$(PATH_TO_TOP)/w32api/include -I$(PATH_TO_TOP)/include -DMEMTRACK

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_ASFLAGS = -I $(PATH_TO_TOP)/include

TARGET_LFLAGS = -Wl,--file-alignment,0x1000 \
  -Wl,--section-alignment,0x1000 \
  -nostartfiles -nostdlib \

TARGET_GCCLIBS = gcc

TARGET_BASE = 0x79f60000

TARGET_PATH = 

REACTOS_OBJECTS = \
	oskittcp/defaults.o \
	oskittcp/in.o \
	oskittcp/in_cksum.o \
	oskittcp/in_pcb.o \
	oskittcp/in_proto.o \
	oskittcp/in_rmx.o \
	oskittcp/inet_ntoa.o \
	oskittcp/interface.o \
	oskittcp/ip_input.o \
	oskittcp/ip_output.o \
	oskittcp/kern_clock.o \
	oskittcp/kern_subr.o \
	oskittcp/malloc.o \
	oskittcp/param.o \
	oskittcp/radix.o \
	oskittcp/random.o \
	oskittcp/rtsock.o \
	oskittcp/scanc.o \
	oskittcp/sleep.o \
	oskittcp/tcp_input.o \
	oskittcp/tcp_output.o \
	oskittcp/tcp_subr.o \
	oskittcp/tcp_usrreq.o \
	oskittcp/tcp_debug.o \
	oskittcp/tcp_timer.o \
	oskittcp/uipc_domain.o \
	oskittcp/uipc_mbuf.o \
	oskittcp/uipc_socket.o \
	oskittcp/uipc_socket2.o 

TARGET_OBJECTS = $(FREEBSD_OBJECTS) $(REACTOS_OBJECTS)
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

%/TAGS:
	etags -o $(@D)/TAGS $(@D)/\*.c

etags: csr/TAGS dbg/TAGS ldr/TAGS main/TAGS rtl/TAGS stdio/TAGS stdlib/TAGS string/TAGS stubs/TAGS
	etags -i csr/TAGS -i dbg/TAGS -i ldr/TAGS -i main/TAGS -i rtl/TAGS -i stdio/TAGS -i stdlib/TAGS -i string/TAGS -i stubs/TAGS


docu:
	doxygen Doxyfile

.PHONY: docu


# EOF
