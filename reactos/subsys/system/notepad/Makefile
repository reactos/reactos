PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = notepad

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API -D_WIN32_IE=0x0501

TARGET_RCFLAGS = -D__USE_W32API -D_WIN32_IE=0x0501

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	comdlg32.a

TARGET_OBJECTS = \
	License_En.o \
	dialog.o \
	license.o \
	main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
