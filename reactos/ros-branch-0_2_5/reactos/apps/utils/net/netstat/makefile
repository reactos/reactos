PATH_TO_TOP = ../../../..

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = netstat

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = \
	-D__USE_W32API \
	-D_WIN32_IE=0x600 \
	-D_WIN32_WINNT=0x501 

TARGET_SDKLIBS = user32.a snmpapi.a iphlpapi.a ws2_32.a

TARGET_OBJECTS = $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
