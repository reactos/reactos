#
#  ReactOS mc
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_OS = NT

TARGET_APPTYPE = console

TARGET_NAME = mc

TARGET_CFLAGS = -I./pc -I./slang -DMC_$(TARGET_OS) -DHAVE_CONFIG_H

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a

MC_OBJS = \
	./src/terms.o \
	./src/user.o \
	./src/file.o \
	./src/listmode.o \
	./src/cmd.o \
	./src/command.o \
	./src/help.o \
	./src/menu.o \
	./src/view.o \
	./src/dir.o \
	./src/info.o \
	./src/widget.o \
	./src/option.o \
	./src/dlg.o \
	./src/panelize.o \
	./src/profile.o \
	./src/util.o \
	./src/dialog.o \
	./src/ext.o \
	./src/color.o \
	./src/layout.o \
	./src/setup.o \
	./src/regex.o \
	./src/hotlist.o \
	./src/tree.o \
	./src/win.o \
	./src/complete.o \
	./src/find.o \
	./src/wtools.o \
	./src/boxes.o \
	./src/background.o \
	./src/main.o \
	./src/popt.o \
	./src/text.o \
	./src/screen.o

PC_OBJS= \
	./pc/slint_pc.o \
	./pc/chmod.o \
        ./pc/drive.o

NT_OBJS = \
	./pc/cons_nt.o \
	./pc/dirent_nt.o \
	./pc/key_nt.o \
	./pc/util_win32.o \
	./pc/util_winnt.o \
	./pc/util_nt.o

SLANG_NT = slw32tty.o

SLANG_OS2 = slos2tty.o

SLANG_OBJS = \
	./slang/slerr.o \
	./slang/slgetkey.o \
	./slang/slsmg.o \
	./slang/slvideo.o \
	./slang/$(SLANG_$(TARGET_OS))

MCEDIT_OBJS = \
	./edit/edit.o \
	./edit/editcmd.o \
	./edit/editdraw.o \
	./edit/editmenu.o \
	./edit/editoptions.o \
	./edit/editwidget.o \
	./edit/syntax.o \
	./edit/wordproc.o

TARGET_OBJECTS = \
	$(MC_OBJS) \
	$(PC_OBJS) \
	$(NT_OBJS) \
	$(SLANG_OBJS) \
	$(MCEDIT_OBJS) \


include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
