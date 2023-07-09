#******************************************************************************
#
#   MAKEFILE FOR VflatD
#
#==============================================================================
#
# Copyright (C) Microsoft Corporation 1990-1993.  All Rights reserved.
#
# goal		Create executables (default).
# clean		Remove all files created by any goal.
#
# DEBUG		=YES	Create debug version and define "DEBUG" (default).
#		=NO	Create non-debug version.
# STRICT	=YES	Define STRICT
#               =       No strict definition (default).

DEVICE      = VflatD
VERSION     = 1.5

NAME    =VflatD
NAMEX   =DVA
EXT	=386
ROOT    =..\..
PBIN	=$(ROOT)\bin
PLIB	=$(ROOT)\lib
PINC	=$(ROOT)\inc
PVER	=$(ROOT)\verinfo\$(LANG)
LOBJ	=$(OS)\$(LANG)
LRES	=res\$(LANG)
LSRC	=src\$(LANG)
OBJS    =$(LOBJ)\vflatd.obj
GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PINC)\$(NAME).inc \
         $(LOBJ)\$(NAMEX).386 $(PBIN)\$(NAMEX).386

LIBS	=

!if "$(DEBUG)" == "NO"
DEF	=
LDEBUG	=
ADEBUG	=
!else
DEF	=-DDEBUG
LDEBUG	=
ADEBUG	=$(DEF)
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

AFLAGS  =$(ADEBUG) -DWIN31
LFLAGS	=$(LDEBUG)

IS_32	=TRUE
IS_OEM	=TRUE
MASM6	=TRUE

!include $(ROOT)\build\project.mk

$(LOBJ)\vflatd.obj: $$(@B).asm $(NAME).inc
        @$(ASM) $(AFLAGS) /Fo$@ $(@B).asm

$(LOBJ)\$(NAME).$(EXT) $(LOBJ)\$(NAME).map:	\
		$(OBJS) $$(@B).def
	@$(LINK) @<<
$(OBJS),
$(@R).$(EXT) $(LFLAGS),
$(@R).map,
$(LIBS),
$(@B).def
<<
        @addhdr $*.$(EXT)

$(LOBJ)\$(NAMEX).386: $(LOBJ)\$(NAME).386
        @copy $(LOBJ)\$(NAME).386 $(LOBJ)\$(NAMEX).386

$(PBIN)\$(NAMEX).386: $(LOBJ)\$(NAME).386
        @copy $(LOBJ)\$(NAME).386 $(PBIN)\$(NAMEX).386
