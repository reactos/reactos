#
#  ReactOS winfile
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = winfile

TARGET_CFLAGS = -D_WIN32_IE=0x0400

TARGET_RCFLAGS = -D_WIN32_IE=0x0400

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	advapi32.a \
	version.a \
	$(COMCTL32_TARGET).a \
	$(COMDLG32_TARGET).a

TARGET_OBJECTS = \
	about.o \
        childwnd.o \
        debug.o \
        dialogs.o \
        drivebar.o \
        entries.o \
        framewnd.o \
        listview.o \
        main.o \
        network.o \
        settings.o \
        shell.o \
        sort.o \
        splitpath.o \
        run.o \
        trace.o \
        treeview.o \
        utils.o \
        worker.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
