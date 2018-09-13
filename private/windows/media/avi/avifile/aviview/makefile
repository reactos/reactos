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
!include $(NTMAKEENV)\makefile.def
!else

NAME	=aviview
EXT	=exe
ROOT	=..\..
PBIN	=$(ROOT)\bin
PLIB	=$(ROOT)\lib
PINC	=$(ROOT)\inc
PVER	=$(ROOT)\verinfo\$(LANG)
LOBJ	=$(OS)\$(LANG)
LRES	=. # res\$(LANG)
OBJ1	=$(LOBJ)\$(NAME).obj $(LOBJ)\audplay.obj $(LOBJ)\cdib.obj
OBJ2	=$(LOBJ)\guid.obj $(LOBJ)\regobjs.obj $(LOBJ)\aviball.obj
OBJ3	=$(LOBJ)\muldiv32.obj
OBJ4	=$(LOBJ)\palmap.obj $(LOBJ)\dibmap.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4)
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym
LIBS    =vfw libw mlibcew mmsystem commdlg shell compobj msacm

!if "$(DEBUG)" == "NO"
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG	=-Zi $(DEF)
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS	=-DVfW -DWIN16 -DWIN31 -DWINVER=0x30a -AM /Zp -Od /GA2 /GEs $(TYPES) $(CDEBUG) -I$(LRES) -Fd$* -Fo$@
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
MFLAGS	=-n
L16FLAGS=/ONERROR:NOEXE/ALIGN:16$(L16DEBUG)
RCFLAGS	=-DAltVfW $(RDEBUG)

IS_16		=TRUE
IS_OEM		=TRUE

!include $(ROOT)\build\project.mk
bogus:
	$(CL) /?

$(LOBJ)\aviview.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\aviball.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _BALL $(@B).c
<<

$(LOBJ)\cdib.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _CDIB $(@B).c
<<

$(LOBJ)\audplay.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _PLAY $(@B).c
<<

$(LOBJ)\guid.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\regobjs.obj:	$$(@B).cpp
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\editstrm.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\dibmap.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _PALMAP $(@B).c
<<

$(LOBJ)\palmap.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _PALMAP $(@B).c
<<

$(LOBJ)\aviclip.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).c
<<

$(LOBJ)\enumfetc.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).c
<<

$(LOBJ)\muldiv32.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\memcopy.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\$(NAME).res:	\
		$(LRES)\$$(@B).rc \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) -I$(LRES) $(LRES)\$(@B).rc

$(LOBJ)\$(NAME).$(EXT) $(LOBJ)\$(NAME).map:	\
		$(OBJS) $$(@B).def $$(@R).res
	@$(LINK16) @<<
$(OBJ1)+
$(OBJ2)+
$(OBJ3)+
$(OBJ4),
$(@R).$(EXT) $(L16FLAGS),
$(@R).map,
$(LIBS),
$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)

!endif
