#
# Copyright (C) Microsoft Corporation 1993.  All Rights reserved.
#
LRES    =.
ROOT	=\nt\private\windows\media\avi
NAME	=avifile
EXT	=dll
OBJ1	=$(LOBJ)\avilib.obj $(LOBJ)\avilibcf.obj $(LOBJ)\classobj.obj $(LOBJ)\device.obj $(LOBJ)\avifile.obj $(LOBJ)\extra.obj
OBJ2    =$(LOBJ)\avisave.obj $(LOBJ)\aviopts.obj $(LOBJ)\avicmprs.obj $(LOBJ)\avifps.obj $(LOBJ)\getframe.obj $(LOBJ)\aviidx.obj
OBJ3    =$(LOBJ)\muldiv32.obj $(LOBJ)\memcopy.obj $(LOBJ)\ole2stub.obj $(LOBJ)\fileshar.obj
OBJ4	=$(LOBJ)\buffer.obj $(LOBJ)\fakefile.obj $(LOBJ)\avimem.obj $(LOBJ)\unmarsh.obj $(LOBJ)\afclip.obj $(LOBJ)\enumfetc.obj $(LOBJ)\editstrm.obj $(LOBJ)\avigraph.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4)
GOALS	=$(NAME).rc $(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PBIN)\$(NAME).reg
LIBS    =mdllcew shell libw mmsystem vfw commdlg compobj msacm ole2

#
# has to be 286 for risc-based emulator
#
WANT_286=1

!if "$(NTDEBUG)" == ""
DEF     =
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG  =$(DEF)
DEBUG   =RETAIL
LOBJ    =retail
!else
DEBUG   =DEBUG
DEF     =-DDEBUG
CDEBUG	=-Zid $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG  =-Zi $(DEF)
LOBJ    =debug
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS  =-GA -GEd -DWINDLL -D_WINDLL -AMw -Oxwti $(TYPES) $(CDEBUG) -I$(LRES) -Fd$* -Fo$@ -Fc$* -DWINVER=0x030a
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=$(RDEBUG)
AFLAGS  =-D?MEDIUM -D?QUIET $(ADEBUG)

IS_16		=TRUE
##	IS_32		=TRUE
IS_OEM		=TRUE

!include $(ROOT)\bin.16\project.mk

$(NAME).rc: res\usa\$(NAME).rc
        copy res\usa

$(PBIN)\$(NAME).reg: $(NAME).reg
	@copy $(@F) $@
	
$(LOBJ)\classobj.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\getframe.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\avilib.obj:	$$(@B).cpp $(NAME).h avireg.h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\avilibcf.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\device.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\avifile.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\extra.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<

$(LOBJ)\fileshar.obj:	$$(@B).c $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).c
<<

$(LOBJ)\avisave.obj:	$$(@B).c $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _SAVE $(@B).c
<<

$(LOBJ)\aviopts.obj:	$$(@B).c $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _OPTIONS $(@B).c
<<

$(LOBJ)\avicmprs.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _SAVE $(@B).cpp
<<

$(LOBJ)\avigraph.obj:	$$(@B).c $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _SAVE $(@B).c
<<

$(LOBJ)\ole2stub.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\muldiv32.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\rlea.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\memcopy.obj:	$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=_TEXT $(@B),$@;

$(LOBJ)\avifps.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).cpp
<<

$(LOBJ)\buffer.obj:	$$(@B).c $(NAME).h
	@$(CL) @<<
$(CFLAGS) -Fc -NT _TEXT $(@B).c
<<

$(LOBJ)\fakefile.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).cpp
<<

$(LOBJ)\avimem.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).cpp
<<

$(LOBJ)\unmarsh.obj:	$$(@B).cpp $(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).cpp
<<

$(LOBJ)\enumfetc.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).c
<<

$(LOBJ)\afclip.obj:	$$(@B).c
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).c
<<

$(LOBJ)\editstrm.obj:	$$(@B).cpp $$(@B).h
	@$(CL) @<<
$(CFLAGS) -NT _CLIP $(@B).cpp
<<

$(LOBJ)\aviidx.obj:	$$(@B).cpp $$(@B).h
	@$(CL) @<<
$(CFLAGS) -NT _TEXT $(@B).cpp
<<



$(LOBJ)\$(NAME).res:	\
		$(LRES)\$$(@B).rc \
		$(LRES)\$$(@B).rcv \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver \
		avireg.h \
		$(LRES)\aviopts.dlg
	@rc $(RCFLAGS) -z -fo$@ -I$(PVER) -I$(LRES) $(LRES)\$(@B).rc

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

$(NAME).map: $(LOBJ)\$(NAME).map
        copy $(LOBJ)\$(NAME).map

$(NAME).$(EXT): $(LOBJ)\$(NAME).$(EXT)
        copy $(LOBJ)\$(NAME).$(EXT)



docs:
    autodoc -x EXTERNAL -rd -o $(NAME).rtf *.c *.cpp
