#
#  ReactOS CMD
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = cmd

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a

TARGET_OBJECTS = \
          cmd.o attrib.o alias.o batch.o beep.o call.o chcp.o choice.o \
          cls.o cmdinput.o cmdtable.o color.o console.o copy.o date.o del.o \
          delay.o dir.o dirstack.o echo.o error.o filecomp.o for.o free.o \
          goto.o history.o if.o internal.o label.o locale.o memory.o misc.o \
          move.o msgbox.o path.o pause.o prompt.o redir.o ren.o screen.o \
          set.o shift.o start.o strtoclr.o time.o timer.o title.o type.o \
          ver.o verify.o vol.o where.o window.o #cmd.coff

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
