PATH_TO_TOP = ../../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = finger

TARGET_SDKLIBS = ws2_32.a

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32_SOCKETS

TARGET_OBJECTS = $(TARGET_NAME).o \
  err.o \
  getopt.o \
  net.o 

TARGET_GCCLIBS = iberty

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
