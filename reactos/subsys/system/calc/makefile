PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = calc

TARGET_CFLAGS = -D__USE_W32API -D_WIN32_IE=0x0501

TARGET_RCFLAGS = -D__USE_W32API -D_WIN32_IE=0x0501

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a

TARGET_GCCLIBS = comctl32

TARGET_OBJECTS = \
	dialog.o \
	stats.o \
	winecalc.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
