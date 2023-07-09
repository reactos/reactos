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

LRES    =.
ROOT	=\nt\private\windows\media\avi

NAME	=msvideo
EXT	=dll
ROOT	=..
PBIN	=$(ROOT)\bin
PLIB	=$(ROOT)\lib
PINC	=$(ROOT)\inc
PVER	=$(ROOT)\verinfo\$(LANG)
OBJS	=$(LOBJ)\libentry.obj $(LOBJ)\video.obj $(LOBJ)\dpmipage.obj $(LOBJ)\init.obj $(LOBJ)\thunk16.obj
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym  x.lib $(PINC)\$(NAME).h $(PINC)\msviddrv.h
LIBS	=libw mdllcew mmsystem $(PLIB)\compman shell $(PLIB)\drawdib $(PLIB)\mciwnd

!if "$(NTDEBUG)" == ""
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
DEBUG   =RETAIL
LOBJ    =retail
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG	=-Zi $(DEF)
DEBUG   =DEBUG
LOBJ    =debug
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS	=-DBUILDDLL -DALTVFW -DWIN16 -DWIN31 -Alnw -Oxwt $(TYPES) $(CDEBUG) -I$(LRES) -Fd$* -Fo$@ -Fc
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=-DALTVFW $(RDEBUG)
MFLAGS	=-n


IS_16           =TRUE
IS_SDK          =TRUE
WANT_286        =TRUE
IS_PRIVATE      =TRUE   # temporaray until shlobj.h is added to sdk

!include $(ROOT)\bin.16\project.mk


$(NAME).rc: res\usa\$(NAME).rc
        copy res\usa


$(LOBJ)\libentry.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=INIT $(@B),$@;

$(LOBJ)\debug.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=MSVIDEO $(@B),$@;

$(LOBJ)\dpmipage.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -I$(DEVROOT)\ddk\inc $(@B),$@;

$(LOBJ)\init.obj:	$$(@B).c $(NAME).h msviddrv.h msvideoi.h $(PVER)\verinfo.h
	@$(CL) @<<
$(CFLAGS) -I$(PVER) -NT INIT $(@B).c
<<

$(LOBJ)\thunk16.obj:	$$(@B).c $(NAME).h msviddrv.h msvideoi.h $(PVER)\verinfo.h vidthunk.h
	@$(CL) @<<
$(CFLAGS) -I$(PVER) -NT thunks $(@B).c
<<

$(LOBJ)\video.obj:	$$(@B).c $(NAME).h msviddrv.h msvideoi.h
	@$(CL) @<<
$(CFLAGS) -NT MSVIDEO $(@B).c
<<

$(LOBJ)\$(NAME).res:	\
		$(LRES)\$$(@B).rc \
		$(LRES)\$$(@B).rcv \
		$(NAME).h \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver \
		$(PINC)\icm.rc
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) -I$(LRES) $(LRES)\$(@B).rc

$(LOBJ)\$(NAME).$(EXT) $(LOBJ)\$(NAME).map:	\
		$(OBJS) $$(@B).def $$(@R).res	\
		$(PLIB)\compman.lib	\
		$(PLIB)\drawdib.lib	\
		$(PLIB)\mciwnd.lib
	@$(LINK16) @<<
$(OBJS),
$(@R).$(EXT) $(L16FLAGS),
$(@R).map,
$(LIBS),
$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)
	@mkpublic $(NAME).def $(NAME)
	@implib $(NAME).lib $(NAME)
    $(MAPSYM) $(MFLAGS) -mo $*.sym $*.map

$(NAME).map: $(LOBJ)\$(NAME).map
        copy $(LOBJ)\$(NAME).map

$(NAME).$(EXT): $(LOBJ)\$(NAME).$(EXT)
        copy $(LOBJ)\$(NAME).$(EXT)

$(PINC)\msvideo.h: msvideo.h
	@copy $(@F) $@

$(PINC)\msviddrv.h: msviddrv.h
	@copy $(@F) $@

x.lib:
	@copy $(NAME).lib   $(PINC)\$(NAME).lib
