# $Id: makefile,v 1.28 2004/10/18 19:11:06 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_REGTESTS = yes

TARGET_BUILDENV_TEST = yes

TARGET_TYPE = export_driver

TARGET_NAME = tcpip

TARGET_PCH = include/precomp.h

# -DMEMTRACK
TARGET_CFLAGS = \
	-D__USE_W32API \
	-DNDIS40 \
	-DMEMTRACK \
	-Wall -Werror \
	-I./include \
	-I$(PATH_TO_TOP)/drivers/lib/oskittcp/include \
	-I$(PATH_TO_TOP)/w32api/include \
	-I$(PATH_TO_TOP)/include

TARGET_DDKLIBS = \
	$(PATH_TO_TOP)/dk/w32/lib/oskittcp.a \
	$(PATH_TO_TOP)/dk/w32/lib/ip.a \
	$(PATH_TO_TOP)/dk/w32/lib/rosrtl.a \
	ndis.a

TARGET_CLEAN = tcpip/*.o datalink/*.o \
	$(PATH_TO_TOP)/dk/w32/lib/ip.a \
	$(PATH_TO_TOP)/dk/w32/lib/oskittcp.a 

TCPIP_OBJECTS    = tcpip/buffer.o \
		   tcpip/dispatch.o \
		   tcpip/fileobjs.o \
		   tcpip/irp.o \
		   tcpip/main.o \
                   tcpip/pool.o 
INFO_OBJECTS     = tcpip/info.o tcpip/ninfo.o tcpip/tinfo.o tcpip/iinfo.o
DATALINK_OBJECTS = datalink/lan.o datalink/loopback.o

TARGET_OBJECTS = \
  $(TCPIP_OBJECTS) \
  $(INFO_OBJECTS) \
  $(DATALINK_OBJECTS)

preall: preclean
	$(MAKE) -C ../../lib/ip
	$(MAKE) -C ../../lib/oskittcp
	$(MAKE) all

preclean:
	$(RM) 	$(PATH_TO_TOP)/dk/w32/lib/ip.a \
		$(PATH_TO_TOP)/dk/w32/lib/oskittcp.a

$(PATH_TO_TOP)/dk/w32/lib/oskittcp.a:
	$(MAKE) -C $(PATH_TO_TOP)/drivers/lib/oskittcp

$(PATH_TO_TOP)/dk/w32/lib/ip.a:
	$(MAKE) -C $(PATH_TO_TOP)/drivers/lib/ip

$(TARGET_OBJECTS):: makefile

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
