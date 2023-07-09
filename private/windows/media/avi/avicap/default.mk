BASE    =avicap
NAME    =$(BASE)32
EXT	=dll
ROOT	=..\..\..
OBJS    =$(BASE)f.obj capinit.obj capavi.obj capdib.obj cappal.obj \
         capmisc.obj capwin.obj  capmci.obj capframe.obj \
         capfile.obj dibmap.obj iaverage.obj capio.obj
LIBS    =kernel32.lib \
         user32.lib \
         gdi32.lib \
         winmm.lib \
         msvfw32.lib \
         msvcrt.lib

GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym $(PLIB)\$(NAME).lib $(PINC)\$(BASE)f.asm $(PINC)\$(BASE).h

!if "$(DEBUG)" == "retail"
DEF	=
TDEBUG	=
ADEBUG	=
L32DEBUG=-debug:none
CDEBUG  =$(DEF) -DCHICAGO
RDEBUG  = -DCHICAGO
!else
!if "$(DEBUG)" == "debug"
DEF	=-DDEBUG_RETAIL
TDEBUG	=
ADEBUG	=
L32DEBUG=-debug:none
CDEBUG  =$(DEF) -DCHICAGO
RDEBUG  = -DCHICAGO
!else
DEF	=-DDEBUG
TDEBUG	=
#ADEBUG	=-Zi $(DEF)
ADEBUG	=
L32DEBUG=-debug:full -debugtype:cv
CDEBUG  =$(DEF) -DCHICAGO
RDEBUG  = -DCHICAGO
!endif
!endif

CFLAGS	=-Oxs -W3 -D_X86_ $(CDEBUG) -I$(PVER) -Fo$@
AFLAGS	=-Zp4 -DSTD_CALL -DBLD_COFF -coff $(ADEBUG)
L32FLAGS=$(L32DEBUG)
RCFLAGS	=$(RDEBUG)

IS_32	=TRUE
IS_OEM	=TRUE
WANT_MASM61=TRUE

!include $(ROOT)\build\project.mk

capinit.obj :  ..\..\$$(@B).c
capavi.obj  :  ..\..\$$(@B).c ..\..\mmtimers.h
capdib.obj  :  ..\..\$$(@B).c
cappal.obj  :  ..\..\$$(@B).c
capmisc.obj :  ..\..\$$(@B).c
capwin.obj  :  ..\..\$$(@B).c
capmci.obj  :  ..\..\$$(@B).c
capframe.obj:  ..\..\$$(@B).c
capfile.obj :  ..\..\$$(@B).c
capio.obj   :  ..\..\$$(@B).c
dibmap.obj  :  ..\..\$$(@B).c
iaverage.obj:  ..\..\$$(@B).c

$(BASE)f.asm:   $$(@B).thk
        @thunk $(TDEBUG) -P2 -NC $(BASE) -t $(BASE)f $(@B).thk -o $@

$(BASE)f.thk:   ..\..\$$(@B).pre
        @$(CL) -nologo $(CDEBUG) /E ..\..\$(@B).pre >$(@B).thk

$(BASE)f.obj:   $$(@R).asm ..\..\$$(@B).inc
        @$(ASM) $(AFLAGS) -I..\.. /Fo$@ $(@B).asm

$(PINC)\$(BASE)f.asm:   $$(@F)
        @copy %s $@

$(NAME).res:	..\..\$$(@B).rc ..\..\$$(@B).rcv \
		$(PVER)\verinfo.h $(PVER)\verinfo.ver ..\..\$(BASE)i.h
	@$(RC) $(RCFLAGS) -fo$@ -I$(PVER) -I..\.. ..\..\$(@B).rc

$(NAME).map $(NAME).lib:	$(@B).$(EXT)

$(NAME).$(EXT):	\
        $(OBJS) $(NAME).res ..\..\$$(@B).def $(PINC)\coffbase.txt
	@$(LINK32) $(L32FLAGS) @<<
-base:@$(PINC)\coffbase.txt,$(NAME)
-merge:.rdata=.text
-merge:.bss=.data
-out:$(@B).$(EXT)
-map:$(@B).map
-dll
-machine:$(OS)
-subsystem:windows,4.0
-entry:_DllMainCRTStartup@12
-implib:$(@B).lib
-def:..\..\$(NAME).def
$(@B).res
$(LIBS)
$(OBJS)
<<
