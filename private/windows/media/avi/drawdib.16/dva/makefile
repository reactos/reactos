#******************************************************************************
#
# MAKEFILE FOR DVA
#
# Copyright (C) Microsoft Corporation 1990-1993.  All Rights reserved.
#
# goal		Create executables (default).
# clean		Remove all files created by any goal.
#
# DEBUG		=YES	Create debug version and define "DEBUG" (default).
#		=NO	Create non-debug version.
# STRICT	=YES	Define STRICT
#		=	No strict definition (default).

!if "$(NTMAKEENV)" != ""
!error	Use BUILD on NT environment!
!endif

NAME    =dva
EXT	=lib
ROOT    =..\..
OBJ1    =$(LOBJ)\dva.obj $(LOBJ)\vflat.obj $(LOBJ)\dvavga.obj $(LOBJ)\dvathun.obj $(LOBJ)\dvavlb.obj
OBJ2    =$(LOBJ)\dvaati.obj $(LOBJ)\dvadib.obj $(LOBJ)\lockbm.obj
OBJ3    =
OBJS    =$(OBJ1) $(OBJ2) $(OBJ3)
GOALS   =$(PLIB)\$(NAME).$(EXT) $(PINC)\$(NAME).h $(PINC)\$(NAME)DDI.h

!if "$(DEBUG)" == "NO"
DEF	=
CDEBUG	=
ADEBUG	=
!else
!if "$(DEBUG)" == "RETAIL"
DEF     =-DDEBUG_RETAIL
CDEBUG  =-Zd $(DEF)
ADEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
ADEBUG	=-Zi $(DEF)
!endif
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS  =-DWIN16 -DWIN31 -Asnw -Oxwti $(TYPES) $(CDEBUG) -Fd$* -Fo$@ -Fc$*
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)

IS_16		=TRUE
IS_SDK		=TRUE
IS_PRIVATE	=TRUE	# temporaray until shlobj.h is added to sdk

!include $(ROOT)\build\project.mk

$(LOBJ)\vflat.obj:   $$(@B).asm
	@echo $(@B).asm
        @$(ASM) $(AFLAGS) -DSEGNAME=DVA -I..\vflatd $(@B),$@;

$(LOBJ)\dva.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<

$(LOBJ)\dvaati.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<

$(LOBJ)\dvavga.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<

$(LOBJ)\dvadib.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<

$(LOBJ)\dvavlb.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<

$(LOBJ)\dvathun.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<



$(LOBJ)\lockbm.obj:    $$(@B).c *.h
	@$(CL) @<<
$(CFLAGS) -NT DVA $(@B).c
<<
