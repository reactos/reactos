# $Id: makefile,v 1.4 2004/10/07 15:04:45 arty Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = library

TARGET_NAME = ip

TARGET_PCH = $(PATH_TO_TOP)/drivers/lib/ip/include/precomp.h

# -DMEMTRACK
TARGET_CFLAGS = \
	-D__USE_W32API \
	-DMEMTRACK \
	-Wall -Werror \
	-Iinclude \
	-I../../net/tcpip/include \
	-I../undis/include \
	-I../oskittcp/include

TARGET_CLEAN = network/*.o

TARGET_OBJECTS = \
	network/address.o \
	network/arp.o \
	network/checksum.o \
	network/i386/checksum.o \
	network/icmp.o \
	network/interface.o \
	network/ip.o \
	network/loopback.o \
	network/memtrack.o \
	network/neighbor.o \
	network/prefix.o \
	network/receive.o \
	network/route.o \
	network/router.o \
	network/routines.o \
	network/transmit.o \
	transport/datagram/datagram.o \
	transport/rawip/rawip.o \
	transport/tcp/event.o \
	transport/tcp/if.o \
	transport/tcp/tcp.o \
	transport/udp/udp.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
