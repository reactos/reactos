#
#  ReactOS regedit
#
#  Makefile
#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = regedit

TARGET_INSTALLDIR = .

TARGET_CFLAGS = -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API
	
TARGET_SDKLIBS = kernel32.a advapi32.a user32.a gdi32.a wine_uuid.a ole32.a \
                 shell32.a comctl32.a comdlg32.a

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
	security.o \
	treeview.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
