NAME	=avicap
EXT	=dll
ROOT	=\nt\private\windows\media\avi
OBJ1	=capavi.obj capinit.obj capdib.obj cappal.obj capdriv.obj capmisc.obj
OBJ2    =capwin.obj capmci.obj capframe.obj capfile.obj dibmap.obj muldiv32.obj
OBJ3    =memcopy.obj libentry.obj iaverage.obj
OBJS	=$(OBJ1) $(OBJ2) $(OBJ3)

GOALS	=$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PLIB)\$(NAME).lib $(PINC)\$(NAME).h
LIBS	=ver libw mdllcew mmsystem vfw

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG	=
L16DEBUG=
RDEBUG	=
ADEBUG	=
!else
!if "$(DEBUG)" == "debug"
DEF	=-DDEBUG_RETAIL
CDEBUG	=-Zd $(DEF)
L16DEBUG=/LI
RDEBUG	=-v $(DEF)
ADEBUG	=$(DEF)
!else
DEF	=-DDEBUG
CDEBUG	=-Zid -Od $(DEF)
L16DEBUG=/CO/LI
RDEBUG	=-v $(DEF)
ADEBUG	=-Zi $(DEF)
!endif
!endif

CFLAGS	=-D_WINDLL -DWIN16 -DWIN31 -Alnw -Oxwt $(CDEBUG) -Fd$* -Fo$@ -GD
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
L16FLAGS=/AL:16/ONERROR:NOEXE$(L16DEBUG)
RCFLAGS	=$(RDEBUG)
MFLAGS	=-n

WANT_286        =TRUE
IS_OEM		=TRUE
IS_16 		=TRUE

!include $(ROOT)\bin.16\project.mk

libentry.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=INIT ..\..\$(@B),$@;

memcopy.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=AVICAP ..\..\$(@B),$@;

muldiv32.obj:	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=AVICAP ..\..\$(@B),$@;

#
# thunk stuff
#
avicapf.obj:    $(PINC)\$$(@B).asm
        @echo $(@B).asm
        mlx -nologo -DIS_16 -D?MEDIUM -D?QUIET $(ADEBUG) -W3 -Zd -c -Cx -DMASM6 -Fo$@ $(PINC)\$(@B).asm

# @$(ASM) $(AFLAGS) -DSEGNAME=THUNK $(PINC)\$(@B),$@;

thunka.obj:    ..\..\$$(@B).asm
        @echo $(@B).asm
        mlx -nologo -DIS_16 -D?MEDIUM -D?QUIET $(ADEBUG) -W3 -Zd -c -Cx -DMASM6 -Fo$@ ..\..\$(@B).asm

# @$(ASM) $(AFLAGS) -DSEGNAME=THUNK ..\..\$(@B),$@;

thkinit.obj:    ..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -NT INIT ..\..\$(@B).c
<<

thunk.obj:    ..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -NT THUNK ..\..\$(@B).c
<<

#
#

capinit.obj:	..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -I$(PVER) -NT INIT ..\..\$(@B).c
<<

dibmap.obj:	..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -NT INIT ..\..\$(@B).c
<<

capmci.obj:	..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capframe.obj:	..\..\$$(@B).c ..\..\$(NAME).h
        @$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

iaverage.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capfile.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT INIT ..\..\$(@B).c
<<

capavi.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capdib.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

cappal.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capwin.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capdriv.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<

capmisc.obj:	..\..\$$(@B).c ..\..\$(NAME).h
	@$(CL) @<<
$(CFLAGS) -NT AVICAP ..\..\$(@B).c
<<


$(NAME).res:	\
		..\..\$$(@B).rc \
		..\..\$$(@B).rcv \
		..\..\$(NAME).h \
		$(PVER)\verinfo.h \
		$(PVER)\verinfo.ver
	@$(RC) $(RCFLAGS) -z -fo$@ -I$(PVER) ..\..\$(@B).rc

$(NAME).$(EXT) $(NAME).map:	\
		$(OBJS) ..\..\$$(@B).def $$(@R).res
	@$(LINK16) @<<
$(OBJ1)+
$(OBJ2)+
$(OBJ3),
$(@R).$(EXT) $(L16FLAGS),
$(@R).map,
$(LIBS),
..\..\$(@B).def
<<
	@$(RC) $(RESFLAGS) $*.res $*.$(EXT)
#       copy  $(NAME).$(EXT) ..\..


api:
    autodoc -x AVICAP_MESSAGE -rd -o $(NAME)m.rtf ..\..\*.d

apistr:
    autodoc -x AVICAP_STRUCTURE -rd -o $(NAME)s.rtf ..\..\*.d
