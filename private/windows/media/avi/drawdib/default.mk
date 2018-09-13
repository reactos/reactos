#DITH    =dith775
DITH    =dith666

NAME	=drawdib
EXT     =lib
ROOT    =..\..\..
!if "$(WIN32)" == "TRUE"
OBJS    =$(NAME).obj $(DITH).obj dither.obj stretchc.obj profdisp.obj setdi.obj lockbm.obj
!else
OBJ1	=$(NAME).obj $(DITH).obj dither.obj dither8.obj
OBJ2    =stretch.obj mapa.obj profdisp.obj $(DITH)a.obj lockbm.obj
OBJ3	=setdi.obj setdi8.obj setdi16.obj setdi24.obj setdi32.obj
OBJS    =$(OBJ1) $(OBJ2) $(OBJ3) 
!endif
GOALS   =$(PLIB)\$(NAME).$(EXT) $(PINC)\$(NAME).h $(PINC)\dith775.h

!if "$(DEBUG)" == "retail"
DEF	=
CDEBUG	=$(DEF)
ADEBUG  =
!else
!if "$(DEBUG)" == "debug"
DEF	=-DDEBUG_RETAIL
CDEBUG	=$(DEF)
ADEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG	=$(DEF)
ADEBUG  =-Zi $(DEF)
!endif
!endif

!if "$(WIN32)" == "TRUE"
CFLAGS  =-Oxs -D_X86_ $(CDEBUG) -Fo$@ -DCHICAGO
IS_32	=TRUE
WANT_C932=TRUE
LB	=lib	# Don't want c816 lib
!else
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)
CFLAGS  =-Oxwti -Asnw -DWIN16 $(CDEBUG) -Fo$@ -DCHICAGO
IS_16	=TRUE
!endif

IS_OEM	=TRUE

!include $(ROOT)\build\project.mk

$(NAME).obj:	..\..\$$(@B).c ..\..\drawdibi.h $(PINC)\profile.h $(PINC)\compman.h $(PINC)\compddk.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

$(DITH).obj:	..\..\$$(@B).c ..\..\drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

dither.obj:	..\..\$$(@B).c ..\..\drawdibi.h $(PINC)\compman.h $(PINC)\compddk.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

stretchc.obj:	..\..\$$(@B).c ..\..\drawdibi.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

profdisp.obj:	..\..\$$(@B).c ..\..\drawdibi.h $(PINC)\profile.h $(PINC)\compman.h $(PINC)\compddk.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

setdi.obj:	..\..\$$(@B).c
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<
!endif

lockbm.obj:     ..\..\$$(@B).c
	@$(CL) @<<
$(CFLAGS) ..\..\$(@B).c
<<

!if "$(WIN32)" != "TRUE"
dith775a.obj $(DITH)a.obj dither8.obj mapa.obj stretch.obj setdi8.obj setdi16.obj setdi24.obj setdi32.obj:	\
	..\..\$$(@B).asm
	@echo $(@B).asm
	@$(ASM) $(AFLAGS) -DSEGNAME=$(NAME)_386 ..\..\$(@B),$@;
!endif

$(PINC)\profile.h:	$(ROOT)\msvideo.32\$$(@F)
	@copy %s $@

$(PINC)\compman.h $(PINC)\compddk.h:	$(ROOT)\compman.32\$$(@F)
	@copy %s $@
