
PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = txtscale

TARGET_NORC = yes

TARGET_CFLAGS = -fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W -D__USE_W32API -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501
	
TARGET_CPPFLAGS = -fexceptions -g -O0 -DWIN32 -D_DEBUG -D_WINDOWS -D_MBCS -W -D__USE_W32API -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a

TARGET_GCCLIBS = comctl32

TARGET_OBJECTS = \
	txtscale.o \
	mk_font.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# overide LD_CC to use g++ for linking of the executable
LD_CC = $(CXX)

# EOF
