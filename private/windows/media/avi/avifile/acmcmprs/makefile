#
# Copyright (C) Microsoft Corporation 1993.  All Rights reserved.
#

!if "$(NTMAKEENV)" != ""
!INCLUDE $(NTMAKEENV)\makefile.def
!else


NAME	=acmcmprs
RESNAME =handler
EXT	=dll
ROOT	=..\..
PBIN	=$(ROOT)\bin
PLIB	=$(ROOT)\lib
PINC	=$(ROOT)\inc
PVER	=$(ROOT)\verinfo\$(LANG)
LOBJ	=$(OS)\$(LANG)
LRES	=res\$(LANG)
OBJS	=$(LOBJ)\factory.obj $(LOBJ)\$(NAME).obj $(LOBJ)\muldiv32.obj
GOALS	=$(PBIN)\$(NAME).sym $(PBIN)\$(NAME).$(EXT)
LIBS	=libw mdllcew shell mmsystem avifile msacm

!if "$(DEBUG)" == "NO"
DEF     =
CDEBUG	=
L16DEBUG=
RDEBUG	=
!else
!if "$(DEBUG)" == "RETAIL"
DEF     =-DDEBUG_RETAIL
CDEBUG  =-Zd $(DEF)
L16DEBUG=/LI
RDEBUG  =-v $(DEF)
!else
DEF     =-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
!endif
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS	=-G2s -GA -GEd -DWINDLL -AMw -Oxws $(TYPES) $(CDEBUG) -I$(LRES) -Fo$(LOBJ)\$(@F)
L16FLAGS=/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=$(RDEBUG)

IS_16		=TRUE
IS_OEM		=TRUE

!include $(ROOT)\build\project.mk

$(LOBJ)\factory.obj:	$$(@B).cpp $(RESNAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\$(NAME).obj:	$$(@B).cpp $(RESNAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\muldiv32.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\$(NAME).res:	\
		$(LRES)\$(RESNAME).rc \
		$(LRES)\$(RESNAME).rcv \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver
	@rc $(RCFLAGS) -z -fo$@ -I$(PVER) -I$(LRES) $(LRES)\$(RESNAME).rc

$(LOBJ)\$(NAME).$(EXT) $(LOBJ)\$(NAME).map:	\
		$(OBJS) $$(@B).def $$(@R).res
	@$(LINK16) @<<
$(OBJS),
$(@R).$(EXT) $(L16FLAGS),
$(@R).map,
$(LIBS),
$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)

!endif
