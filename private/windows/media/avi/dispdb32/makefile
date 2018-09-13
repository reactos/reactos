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
!INCLUDE $(NTMAKEENV)\makefile.def
####!error	Use BUILD on NT environment!
!else

NAME	=mciavi
EXT	=drv
ROOT	=..
PBIN	=$(ROOT)\bin
PLIB	=$(ROOT)\lib
PINC	=$(ROOT)\inc
PVER	=$(ROOT)\verinfo\$(LANG)
LOBJ	=$(OS)\$(LANG)
LRES	=res\$(LANG)
OBJ1	=$(LOBJ)\libinit.obj $(LOBJ)\graphic.obj $(LOBJ)\window.obj $(LOBJ)\device.obj $(LOBJ)\drvproc.obj $(LOBJ)\math.obj
OBJ2	=$(LOBJ)\common.obj $(LOBJ)\playrle.obj $(LOBJ)\rlea.obj $(LOBJ)\config.obj $(LOBJ)\avitask.obj $(LOBJ)\avidraw.obj $(LOBJ)\ole2nlss.obj
OBJ3    =$(LOBJ)\avisound.obj $(LOBJ)\aviplay.obj $(LOBJ)\aviopen.obj $(LOBJ)\drawproc.obj $(LOBJ)\fullproc.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3)
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PINC)\$(NAME).h $(PINC)\aviffmt.h
LIBS	=libw mdllcew mmsystem msvideo

!if "$(DEBUG)" == "NO"
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
!else
!if "$(DEBUG)" == "RETAIL"
DEF     =-DDEBUG_RETAIL
CDEBUG  =-Zd $(DEF)
L16DEBUG=/LI
RDEBUG  =-v $(DEF)
ADEBUG  =$(DEF)
OBJD    =
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG	=-Zi $(DEF)
!endif
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS  =-DALTVFW -DWIN16 -DWIN31 -Alnw -Oxzw $(TYPES) $(CDEBUG) -I$(LRES) -Fc$*.cod -Fd$* -Fo$@ -DWINVER=0x30a
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=-DALTVFW $(RDEBUG)
MFLAGS	=-n

IS_16		=TRUE
IS_OEM		=TRUE
# WANT_286	=1

!include $(ROOT)\build\project.mk

## !!!! CFLAGS = $(CFLAGS) -G3

$(LOBJ)\libinit.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\math.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\hmemcpy.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\rlea.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=RLE $(@B),$@;

$(LOBJ)\ole2nlss.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\aviopen.obj:	$$(@B).c nocrap.h gmem.h common.h ntaviprt.h playrle.h rlec.h avitask.h aviffmt.h oldavift.h
	@$(CL) @<<
$(CFLAGS) -NT OPEN $(@B).c
<<

$(LOBJ)\graphic.obj:	$$(@B).c nocrap.h ntaviprt.h common.h graphic.h mciavi.h cnfgdlg.h aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\window.obj:	$$(@B).c ntaviprt.h common.h graphic.h mciavi.h aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\device.obj:	$$(@B).c nocrap.h ntaviprt.h common.h graphic.h mciavi.h avitask.h gmem.h aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\drvproc.obj:    $$(@B).c ntaviprt.h common.h graphic.h mciavi.h cnfgdlg.h aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\config.obj:	$$(@B).c ntaviprt.h common.h graphic.h mciavi.h cnfgdlg.h aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT CONFIG $(@B).c
<<

$(LOBJ)\avitask.obj:	$$(@B).c nocrap.h gmem.h ntaviprt.h common.h graphic.h mciavi.h avitask.h aviffmt.h oldavift.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\avidraw.obj:	$$(@B).c nocrap.h gmem.h common.h ntaviprt.h graphic.h mciavi.h playrle.h rlec.h avitask.h aviffmt.h oldavift.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\avisound.obj:	$$(@B).c nocrap.h gmem.h common.h ntaviprt.h graphic.h mciavi.h avitask.h aviffmt.h oldavift.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\aviplay.obj:	$$(@B).c nocrap.h gmem.h common.h ntaviprt.h graphic.h mciavi.h playrle.h rlec.h avitask.h aviffmt.h oldavift.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\common.obj:	$$(@B).c ntaviprt.h common.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\playrle.obj:	$$(@B).c ntaviprt.h gmem.h playrle.h
	@$(CL) @<<
$(CFLAGS) -NT RLE $(@B).c
<<

$(LOBJ)\drawproc.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\fullproc.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\$(NAME).res:	$(LRES)\$$(@B).rc $(LRES)\$$(@B).rcv \
		graphic.h $(NAME).h cnfgdlg.h $(LRES)\cnfgdlg.dlg \
		$(PVER)\verinfo.h $(PVER)\verinfo.ver
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) -I$(LRES) $(LRES)\$(@B).rc

$(LOBJ)\$(NAME).$(EXT) $(LOBJ)\$(NAME).map:	\
		$(OBJS) $$(@B).def $$(@R).res
	@$(LINK16) @<<
$(OBJ1)+
$(OBJ2)+
$(OBJ3),
$(@R).$(EXT) $(L16FLAGS),
$(@R).map,
$(LIBS),
$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)
!endif
