# $Id: makefile,v 1.17 2004/06/09 18:11:37 arty Exp $

PATH_TO_TOP = ../../..

TARGET_REGTESTS = yes

TARGET_TYPE = export_driver

TARGET_NAME = tcpip

TARGET_CFLAGS = \
	-D__USE_W32API \
	-DNDIS40 \
	-DMEMTRACK \
	-I./include \
	-I$(PATH_TO_TOP)/drivers/lib/oskittcp/include \
	-I$(PATH_TO_TOP)/w32api/include \
	-I$(PATH_TO_TOP)/include

TARGET_DDKLIBS = ndis.a $(PATH_TO_TOP)/dk/w32/lib/oskittcp.a

TARGET_CLEAN = \
  tcpip/*.o \
  datalink/*.o \
  network/*.o \
  transport/datagram/*.o \
  transport/rawip/*.o \
  transport/tcp/*.o \
  transport/udp/*.o 

TCPIP_OBJECTS    = tcpip/main.o tcpip/address.o tcpip/checksum.o \
                   tcpip/dispatch.o tcpip/fileobjs.o \
                   tcpip/pool.o tcpip/routines.o tcpip/interface.o \
		   tcpip/memtrack.o tcpip/irp.o
INFO_OBJECTS     = tcpip/info.o tcpip/ninfo.o tcpip/tinfo.o tcpip/iinfo.o
DATALINK_OBJECTS = datalink/arp.o datalink/lan.o datalink/loopback.o
NETWORK_OBJECTS  = network/icmp.o network/ip.o network/neighbor.o \
                   network/receive.o network/route.o network/router.o \
                   network/transmit.o network/prefix.o
DATAGRAM_OBJECTS = transport/datagram/datagram.o
RAWIP_OBJECTS    = transport/rawip/rawip.o
TCP_OBJECTS      = transport/tcp/tcp.o transport/tcp/event.o transport/tcp/if.o
UDP_OBJECTS      = transport/udp/udp.o
ARCH_OBJECTS     = tcpip/i386/checksum.o

TARGET_OBJECTS = \
  $(TCPIP_OBJECTS) \
  $(INFO_OBJECTS) \
  $(DATALINK_OBJECTS) \
  $(NETWORK_OBJECTS) \
  $(DATAGRAM_OBJECTS) \
  $(RAWIP_OBJECTS) \
  $(TCP_OBJECTS) \
  $(FREEBSD_OBJECTS) \
  $(REACTOS_OBJECTS) \
  $(UDP_OBJECTS) \
  $(ARCH_OBJECTS) \

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
