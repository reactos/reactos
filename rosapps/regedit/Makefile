#
#  ReactOS regedit
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = regedit

#TARGET_CFLAGS = -I$(TEST_ROOT) -DDBG -DUNICODE -D_UNICODE
#TARGET_CFLAGS = -D_WIN32_IE=0x0400
#TARGET_RCFLAGS = -D_WIN32_IE=0x0400

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	$(COMCTL32_TARGET).a \
	$(COMDLG32_TARGET).a

TARGET_OBJECTS = \
	about.o \
        childwnd.o \
        framewnd.o \
        hex_str.o \
        listview.o \
        main.o \
	regcmds.o \
	regproc.o \
        treeview.o \
        trace.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
