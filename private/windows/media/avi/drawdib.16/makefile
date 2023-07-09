#
# Copyright (C) Microsoft Corporation 1990-1993.  All Rights reserved.
#
# goal          Create executables (default).
# clean         Remove all files created by any goal.
#
# DEBUG         =YES    Create debug version and define "DEBUG" (default).
#               =NO     Create non-debug version.
# STRICT        =YES    Define STRICT
#               =       No strict definition (default).

#DITH    =dith775
DITH    =dith666

LRES    =.
ROOT	=\nt\private\windows\media\avi
LANG    =usa

NAME    =drawdib
EXT     =lib
OBJ1    =$(LOBJ)\drawdib.obj $(LOBJ)\$(DITH).obj $(LOBJ)\dither.obj $(LOBJ)\dither8.obj
OBJ2    =$(LOBJ)\stretch.obj $(LOBJ)\mapa.obj $(LOBJ)\profdisp.obj  $(LOBJ)\$(DITH)a.obj $(LOBJ)\lockbm.obj
OBJ3    =$(LOBJ)\setdi.obj $(LOBJ)\setdi8.obj $(LOBJ)\setdi16.obj $(LOBJ)\setdi24.obj $(LOBJ)\setdi32.obj
OBJS    =$(OBJ1) $(OBJ2) $(OBJ3) dcilib.lib
GOALS   = $(PLIB)\$(NAME).$(EXT) $(PINC)\$(NAME).h $(PINC)\dith775.h

!if "$(NTDEBUG)" == ""
DEF     =
CDEBUG  =-DWINVER=0x0310
ADEBUG  =
DEBUG   =RETAIL
LOBJ    =retail
!else
DEF     =-DDEBUG
CDEBUG  =-Zid $(DEF) -Fc -DWINVER=0x0310
ADEBUG  =-Zi $(DEF)
DEBUG   =DEBUG
LOBJ    =debug
!endif

!if "$(STRICT)" == "YES"
TYPES   =-DSTRICT
!else
TYPES   =
!endif

AFLAGS  =-D?MEDIUM -D?QUIET $(ADEBUG) -l
CFLAGS  =-DWIN16 -DWIN31 -Asnw -Oxwti $(TYPES) $(CDEBUG) -Fd$* -Fo$@

IS_16           =TRUE
IS_SDK          =TRUE
WANT_286        =TRUE
IS_PRIVATE      =TRUE   # temporaray until shlobj.h is added to sdk

!include $(ROOT)\bin.16\project.mk

CFLAGS  =$(CFLAGS)
CFLAGS  =$(CFLAGS) -Idva -Idcilib

DVA:
	cd dva
	$(MAKE)
	cd ..

VFlatD:
	cd vflatd
	$(MAKE)
	cd ..

ddtest:
	cd ddtest
	$(MAKE)
	cd ..

$(LOBJ)\dith775a.obj:   $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\dith666a.obj:   $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\dither8.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\mapa.obj:       $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\stretch.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\profdisp.obj:   $$(@B).c drawdibi.h drawdib.h dither.h stretch.h profdisp.h
	@$(CL) @<<
$(CFLAGS) -NT DDSTUFF $(@B).c
<<

$(LOBJ)\dither.obj:     $$(@B).c dither.h drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h
	@$(CL) @<<
$(CFLAGS) -NT DDSTUFF $(@B).c
<<

$(LOBJ)\dith666.obj:    $$(@B).c dither.h dith666.h drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h
	@$(CL) @<<
$(CFLAGS) -NT DRAWDIB $(@B).c
<<

$(LOBJ)\dith775.obj:    $$(@B).c dither.h dith775.h drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h
	@$(CL) @<<
$(CFLAGS) -NT DRAWDIB $(@B).c
<<

$(LOBJ)\drawdib.obj:    $$(@B).c drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h drawdib.h dither.h stretch.h
	@$(CL) @<<
$(CFLAGS) -NT DRAWDIB $(@B).c
<<

$(LOBJ)\setdi.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DRAWDIB $(@B).c
<<

$(LOBJ)\lockbm.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DRAWDIB $(@B).c
<<

$(LOBJ)\setdi8.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\setdi16.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\setdi24.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(LOBJ)\setdi32.obj:    $$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=DRAWDIB_386 $(@B),$@;

$(PINC)\compman.h $(PINC)\compddk.h:	..\compman\$$(@F)
	@copy ..\compman\$(@F) $@

$(PINC)\$(NAME).h: $(NAME).h
        @copy drawdib.h $@

$(PINC)\dith775.h: dith775.h
        @copy dith775.h $@

