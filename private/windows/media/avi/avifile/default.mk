BASE	=avifile
!if "$(WIN32)" == "TRUE"
NAME	=avifil32
!else
NAME	=$(BASE)
!endif
EXT	=dll
ROOT	=..\..\..
OBJ1	=avilib.obj avilibcf.obj classobj.obj device.obj avifile.obj extra.obj
OBJ2	=avisave.obj aviopts.obj avicmprs.obj avifps.obj getframe.obj aviidx.obj acmcmprs.obj
OBJ3	=fileshar.obj wavefile.obj
OBJ4	=buffer.obj fakefile.obj avimem.obj unmarsh.obj afclip.obj enumfetc.obj editstrm.obj avigraph.obj
!if "$(WIN32)" == "TRUE"
OBJA    =disk32.obj directio.obj
LIBS	=kernel32.lib user32.lib crtdll.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib winmm.lib advapi32.lib msvfw32.lib msacm32.lib uuid.lib 
!else
OBJA    =rlea.obj muldiv32.obj memcopy.obj compobj.obj
LIBS	=mdllcew shell libw mmsystem msvideo commdlg msacm
!endif
OBJS	=$(OBJA) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4)
GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PLIB)\$(NAME).lib $(PINC)\$(BASE).h $(PINC)\aviiface.h

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG  =$(DEF)
L32DEBUG=-debug:none
L16DEBUG=
RDEBUG	=
ADEBUG  =$(DEF)
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =$(DEF)
L32DEBUG=-debug:none
L16DEBUG=/LI
RDEBUG  =-v $(DEF)
ADEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG  =$(DEF)
L32DEBUG=-debug:full -debugtype:cv
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG  =-Zi $(DEF)
!endif
!endif

!if "$(WIN32)" == "TRUE"
CFLAGS  =-Oxs -D_X86_ $(CDEBUG) -Fo$@ -DCHICAGO -DUSE_DIRECTIO -DSHELLOLE
L32FLAGS=$(L32DEBUG)
RCFLAGS	=$(RDEBUG)
IS_32	=TRUE
WANT_C932=TRUE
OS	=i386
LB	=lib	# Don't want c816 lib
!else
CFLAGS  =-Gs -GA -GEd -AMw -Oxwti $(CDEBUG) -Fo$@ -DCHICAGO -DSHELLOLE
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=-z $(RDEBUG)
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
IS_16	=TRUE
!endif

IS_OEM	=TRUE

!include $(ROOT)\build\project.mk

compobj.obj:	..\..\$$(@B).cpp 
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

disk32.obj:   ..\..\$$(@B).c ..\..\disk32.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).c
<<
!else
        $(CL) @<<
$(CFLAGS) -Fc ..\..\$(@B).c
<<
!endif

classobj.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h ..\..\avifps.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

getframe.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

avilib.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h ..\..\avireg.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

avilibcf.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

device.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

$(BASE).obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

extra.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

fileshar.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

avisave.obj:	..\..\$$(@B).c ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _SAVE ..\..\$(@B).c
<<
!endif

wavefile.obj:	..\..\$$(@B).c ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _WAVE ..\..\$(@B).c
<<
!endif

aviopts.obj:	..\..\$$(@B).c ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _OPTIONS ..\..\$(@B).c
<<
!endif

avicmprs.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _SAVE ..\..\$(@B).cpp
<<
!endif

acmcmprs.obj:	..\..\$$(@B).cpp ..\..\acmcmprs.h ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _SAVE ..\..\$(@B).cpp
<<
!endif

avigraph.obj:	..\..\$$(@B).c ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _SAVE ..\..\$(@B).c
<<
!endif

avifps.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h ..\..\avifps.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).cpp
<<
!endif

buffer.obj:	..\..\$$(@B).c ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -Fc -NT _TEXT ..\..\$(@B).c
<<
!endif

fakefile.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).cpp
<<
!endif

avimem.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).cpp
<<
!endif

unmarsh.obj:	..\..\$$(@B).cpp ..\..\$(BASE).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).cpp
<<
!endif

enumfetc.obj:	..\..\$$(@B).c
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).c
<<
!endif

afclip.obj:	..\..\$$(@B).c
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).c
<<
!endif

editstrm.obj:	..\..\$$(@B).cpp ..\..\$$(@B).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _CLIP ..\..\$(@B).cpp
<<
!endif

aviidx.obj:	..\..\$$(@B).cpp ..\..\$$(@B).h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT _TEXT ..\..\$(@B).cpp
<<
!endif

!if "$(WIN32)" != "TRUE"
ole2stub.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

muldiv32.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

rlea.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;

memcopy.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT ..\..\$(@B),$@;
!endif

$(BASE).res:    \
		..\..\$(BASE).rc ..\..\$(BASE).rcv ..\..\$(BASE).h \
		$(PVER)\verinfo.h $(PVER)\verinfo.ver ..\..\aviopts.dlg
	@$(RC) $(RCFLAGS) -fo$@ -I$(PVER) ..\..\$(@B).rc

!if "$(WIN32)" == "TRUE"
$(NAME).lib $(NAME).$(EXT) $(NAME).map: \
	$(OBJS) $(BASE).res ..\..\$(NAME).def $(PINC)\coffbase.txt
	@$(LINK32) $(L32FLAGS) @<<
-merge:.rdata=.text
-merge:.bss=.data
-out:$(@B).$(EXT)
-machine:$(OS)
-subsystem:windows,4.0
-base:@$(PINC)\coffbase.txt,$(NAME)
-map:$(@B).map
-def:..\..\$(NAME).def
-dll
-entry:DLLEntryPoint@12
-implib:$(@B).lib
$(BASE).res
$(OBJA)
$(OBJ1)
$(OBJ2)
$(OBJ3)
$(OBJ4)
$(LIBS)
<<
!else
$(NAME).$(EXT) $(NAME).map:	\
	$(OBJS) ..\..\$$(@B).def $(BASE).res
	@$(LINK16) $(L16FLAGS) @<<
$(OBJA)+
$(OBJ1)+
$(OBJ2)+
$(OBJ3)+
$(OBJ4),
$(@B).$(EXT),
$(@B).map,
$(LIBS),
..\..\$(@B).def
<<
	@$(RC) $(RESFLAGS) $(BASE).res $*.$(EXT)
!endif
