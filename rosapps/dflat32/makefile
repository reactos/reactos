#
#  ReactOS D-Flat/32
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = edit

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

#WITH_DEBUGGING=yes

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a

TARGET_OBJECTS = \
	applicat.o barchart.o box.o button.o calendar.o checkbox.o clipbord.o \
        combobox.o config.o console.o decomp.o dfalloc.o dialbox.o dialogs.o \
        direct.o edit.o editbox.o fileopen.o helpbox.o htree.o keys.o \
        listbox.o lists.o log.o menu.o menubar.o menus.o message.o msgbox.o \
        normal.o pictbox.o popdown.o radio.o rect.o search.o slidebox.o spinbutt.o \
        statbar.o sysmenu.o text.o textbox.o video.o watch.o window.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
