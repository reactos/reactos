# $Id: makefile,v 1.12 2003/01/15 21:57:31 chorns Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = tcpip

TARGET_CFLAGS = -I./include -DDBG

TARGET_DDKLIBS = ndis.a

TARGET_CLEAN = \
  tcpip/*.o \
  datalink/*.o \
  network/*.o \
  transport/datagram/*.o \
  transport/rawip/*.o \
  transport/tcp/*.o \
  transport/udp/*.o

TCPIP_OBJECTS    = tcpip/main.o tcpip/address.o tcpip/checksum.o \
                   tcpip/dispatch.o tcpip/fileobjs.o tcpip/info.o \
                   tcpip/pool.o tcpip/routines.o
DATALINK_OBJECTS = datalink/arp.o datalink/lan.o datalink/loopback.o
NETWORK_OBJECTS  = network/icmp.o network/ip.o network/neighbor.o \
                   network/receive.o network/route.o network/router.o \
                   network/transmit.o
DATAGRAM_OBJECTS = transport/datagram/datagram.o
RAWIP_OBJECTS    = transport/rawip/rawip.o
TCP_OBJECTS      = transport/tcp/tcp.o transport/tcp/tcpcore.o \
                   transport/tcp/tcp_input.o transport/tcp/tcp_ipv4.o \
                   transport/tcp/tcp_output.o transport/tcp/tcp_timer.o
UDP_OBJECTS      = transport/udp/udp.o

TARGET_OBJECTS = \
  $(TCPIP_OBJECTS) \
  $(DATALINK_OBJECTS) \
  $(NETWORK_OBJECTS) \
  $(DATAGRAM_OBJECTS) \
  $(RAWIP_OBJECTS) \
  $(TCP_OBJECTS) \
  $(UDP_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
