BASE	=msvideo
!if "$(WIN32)" == "TRUE"
NAME    =msvfw32
!else
NAME    =$(BASE)
!endif
EXT	=dll
ROOT    =..\..\..
OBJ1	=video.obj init.obj debug.obj
!if "$(WIN32)" == "TRUE"
OBJS    =$(OBJ1) profile.obj
LIBS	=kernel32.lib user32.lib crtdll.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib advapi32.lib winmmi.lib mpr.lib dciman32.lib version.lib
!else
OBJS	=libentry.obj dpmipage.obj $(OBJ1)
LIBS	=libw mdllcew mmsystem shell commctrl dciman ver
!endif
LIBS	=$(LIBS) compman.lib drawdib.lib mciwnd.lib 
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PLIB)\$(NAME).lib $(PINC)\$(BASE).h $(PINC)\msviddrv.h

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG  =$(DEF)
L16DEBUG=
L32DEBUG=-debug:none
RDEBUG	=
ADEBUG  =
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =$(DEF)
L16DEBUG=/LI
L32DEBUG=-debug:none
RDEBUG  =-v $(DEF)
ADEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG  =$(DEF)
L16DEBUG=/CO/LI
L32DEBUG=-debug:full -debugtype:cv
RDEBUG  =-v $(DEF)
ADEBUG  =-Zi $(DEF)
!endif
!endif

!if "$(WIN32)" == "TRUE"
CFLAGS	=-Oxs -D_X86_ $(CDEBUG) -I$(PVER) -Fo$@
IS_32	=TRUE
WANT_C932=TRUE
LB	=lib	# Don't want c816 lib
RCFLAGS	=$(RDEBUG)
!else
CFLAGS	=-Fc -Oxwt -Alnw -DBUILDDLL -D_WINDLL -DWIN16 $(CDEBUG) -I$(PVER) -Fo$@
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=-z $(RDEBUG)
IS_16	=TRUE
!endif

IS_OEM	=TRUE

!include $(ROOT)\build\project.mk

!if "$(WIN32)" != "TRUE"
INCLUDE	=$(INCLUDE);$(DEVROOT)\ddk\inc

libentry.obj:	..\..\$$(@B).asm
        $(ASM) $(AFLAGS) -DSEGNAME=INIT ..\..\$(@B),$@;

dpmipage.obj:	..\..\$$(@B).asm
        $(ASM) $(AFLAGS) -DSEGNAME=$(BASE) ..\..\$(@B),$@;
!endif

init.obj:	..\..\$$(@B).c ..\..\$(BASE).h ..\..\msviddrv.h ..\..\$(BASE)i.h ..\..\debug.h $(PVER)\verinfo.h ..\..\profile.h
!if "$(WIN32)" != "TRUE"
        $(CL) @<<
$(CFLAGS) -NT INIT
..\..\$(@B).c
<<
!endif

video.obj:	..\..\$$(@B).c ..\..\$(BASE).h ..\..\msviddrv.h ..\..\$(BASE)i.h ..\..\profile.h ..\..\debug.h
!if "$(WIN32)" != "TRUE"
        $(CL) @<<
$(CFLAGS) -NT $(BASE)
..\..\$(@B).c
<<
!endif

profile.obj:	..\..\$$(@B).c ..\..\$(BASE).h ..\..\profile.h
!if "$(WIN32)" != "TRUE"
        $(CL) @<<
$(CFLAGS) -NT $(BASE)
..\..\$(@B).c
<<
!endif

debug.obj:	..\..\$$(@B).c ..\..\debug.h
!if "$(WIN32)" != "TRUE"
        $(CL) @<<
$(CFLAGS) -NT $(BASE)
..\..\$(@B).c
<<
!endif


$(BASE).res:	\
		..\..\$$(@B).rc \
		..\..\$$(@B).rcv \
		..\..\$(BASE).h \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver \
		$(PINC)\icm.rc
        $(RC) $(RCFLAGS) -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

!if "$(WIN32)" == "TRUE"
$(NAME).lib $(NAME).$(EXT) $(NAME).map: \
	$(OBJS) $(BASE).res ..\..\$(NAME)c.def \
 	$(PLIB)\compman.lib	\
 	$(PLIB)\drawdib.lib	\
 	$(PLIB)\mciwnd.lib	\
	$(PLIB)\dciman32.lib $(PINC)\coffbase.txt
        $(LINK32) $(L32FLAGS) $(L32DEBUG) @<<
-out:$(@B).$(EXT)
-machine:$(OS)
-subsystem:windows,4.0
-base:@$(PINC)\coffbase.txt,$(NAME)
-map:$(@B).map
-def:..\..\$(NAME)c.def
-dll
-entry:DLLEntryPoint@12
-implib:$(@B).lib
$(BASE).res
$(OBJS)
$(LIBS)
<<
!else
$(NAME).$(EXT) $(NAME).map:	\
	$(OBJS) ..\..\$$(@B)c.def $(BASE).res	\
	$(PLIB)\compman.lib	\
	$(PLIB)\drawdib.lib	\
	$(PLIB)\mciwnd.lib	\
	$(PLIB)\dciman.lib
        $(LINK16) $(L16FLAGS) @<<
$(OBJS),
$(@B).$(EXT),
$(@B).map,
$(LIBS),
..\..\$(@B)c.def
<<
        $(RC) $(RESFLAGS) $(BASE).res $*.$(EXT)
!endif
