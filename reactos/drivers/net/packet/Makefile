# $Id: Makefile,v 1.4 2003/02/21 16:30:05 robd Exp $

PATH_TO_TOP = ../../..

#TARGET_TYPE = export_driver
TARGET_TYPE = driver

TARGET_NAME = packet

#TARGET_CFLAGS = -DDBG -DWIN_NT_DRIVER -DKQPC_TS -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_CFLAGS = -DDBG -DWIN_NT_DRIVER -DKQPC_TS -DUSE_KLOCKS -I$(PATH_TO_TOP)/ntoskrnl/include

TARGET_DDKLIBS = ndis.a

TARGET_OBJECTS = \
	packet.o    \
	openclos.o  \
	read.o      \
	write.o     \
	dump.o      \
	jitter.o      \
	win_bpf_filter.o \
	tme.o \
	count_packets.o \
	win_bpf_filter_init.o \
	tcp_session.o \
	memory_t.o \
	time_calls.o \
	functions.o \
	bucket_lookup.o \
	normal_lookup.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
