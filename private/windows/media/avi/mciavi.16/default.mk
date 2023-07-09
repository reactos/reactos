NAME	=mciavi
EXT	=drv
ROOT	=\nt\private\windows\media\avi
OBJ1    =libinit.obj graphic.obj window.obj device.obj drvproc.obj
OBJ2    =common.obj config.obj avitask.obj avidraw.obj math.obj
OBJ3    =avisound.obj aviplay.obj aviopen.obj drawproc.obj fullproc.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3)
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PINC)\$(NAME).h $(PINC)\aviffmt.h
LIBS    =libw mdllcew mmsystem vfw

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
!else
!if "$(DEBUG)" == "debug"
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

CFLAGS  =-DWIN16 -Alnw -Oxwt $(CDEBUG) -Fd$* -Fo$@
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=$(RDEBUG)
MFLAGS	=-n

IS_16		=TRUE
IS_OEM		=TRUE
WANT_286	=1

!include $(ROOT)\bin.16\project.mk

## !!!! CFLAGS = $(CFLAGS) -G3

libinit.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

math.obj:    ..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

aviopen.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\avitask.h ..\..\aviffmt.h ..\..\graphic.h
	@$(CL) @<<
$(CFLAGS) -NT OPEN ..\..\$(@B).c
<<

graphic.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\cnfgdlg.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

window.obj:     ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

device.obj:     ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\avitask.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

drvproc.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\cnfgdlg.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

config.obj:     ..\..\$$(@B).c  ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\cnfgdlg.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT CONFIG ..\..\$(@B).c
<<

avitask.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\avitask.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

avidraw.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\avitask.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

avisound.obj:   ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\avitask.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

aviplay.obj:    ..\..\$$(@B).c ..\..\common.h ..\..\graphic.h ..\..\mciavi.h ..\..\avitask.h ..\..\aviffmt.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

common.obj:     ..\..\$$(@B).c ..\..\common.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

drawproc.obj:	..\..\$$(@B).c $(PINC)\avicap.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

fullproc.obj:	..\..\$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<

$(NAME).res:	..\..\$$(@B).rc ..\..\$$(@B).rcv \
		..\..\graphic.h ..\..\$(NAME).h ..\..\cnfgdlg.h ..\..\mciavi.mci ..\..\cnfgdlg.dlg \
 		$(PVER)\verinfo.h $(PVER)\verinfo.ver ..\..\people.cry
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map:	\
		$(OBJS) ..\..\$$(@B).def $$(@B).res
	@$(LINK16) @<<
$(OBJ1)+
$(OBJ2)+
$(OBJ3),
$(@B).$(EXT) $(L16FLAGS),
$(@B).map,
$(LIBS),
..\..\$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)

..\..\$(NAME).rc: ..\..\res\usa\$(NAME).rc ..\..\res\usa\$(NAME).rcv    \
                  ..\..\res\usa\cnfgdlg.dlg
        copy ..\..\res\usa ..\..
