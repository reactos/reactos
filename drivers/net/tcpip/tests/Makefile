PATH_TO_TOP = ../../../..

TARGET_TYPE = test

TARGET_NAME = regtests

TARGET_PCH =

TARGET_LIBPATH = .

TARGET_LIBS = \
	../tcpip/main.o \
	../tcpip/dispatch.o \
	../tcpip/irp.o \
	../tcpip/fileobjs.o \
	../tcpip/proto.o \
	../tcpip/mockbuffer.o \
	../tcpip/mocklock.o \
	../tcpip/mockwait.o \
	../tcpip/mockpool.o \
	../tcpip/mockbug.o \
	../tcpip/info.o \
	../tcpip/ninfo.o \
	../tcpip/iinfo.o \
	../tcpip/tinfo.o \
	../datalink/lan.o \
	$(PATH_TO_TOP)/dk/w32/lib/ip.a \
	$(PATH_TO_TOP)/dk/w32/lib/oskittcp.a 

TARGET_CFLAGS = \
	-I$(REGTESTS_PATH_INC) \
	-D__USE_W32API \
	-DNDIS40 \
	-DMEMTRACK \
	-Wall -Werror \
	-I../include \
	-I$(PATH_TO_TOP)/drivers/lib/oskittcp/include \
	-I$(PATH_TO_TOP)/w32api/include \
	-I$(PATH_TO_TOP)/include

-include Makefile.tests

TARGET_OBJECTS = \
  setup.o \
  $(addprefix tests/, $(TESTS))

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
