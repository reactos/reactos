#
#  ReactOS regedit
#
#  Makefile
#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = regedit

TARGET_CFLAGS = -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API
	
TARGET_GCCLIBS = msvcrt advapi32 kernel32 comctl32 comdlg32 shell32

TARGET_OBJECTS = \
	about.o \
	childwnd.o \
	edit.o \
	framewnd.o \
	hexedit.o \
	listview.o \
	main.o \
	regedit.o \
	regproc.o \
	treeview.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
