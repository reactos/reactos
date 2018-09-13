NAME	=compman
EXT	=lib
ROOT	=..\..\..
OBJS	=compman.obj icm.obj thunk.obj
LIBS	=
GOALS	=$(PLIB)\$(NAME).$(EXT) $(PINC)\$(NAME).h $(PINC)\compddk.h $(PINC)\icm.rc

!if "$(DEBUG)" == "retail"
DEF     =
CDEBUG  =$(DEF)
!else
!if "$(DEBUG)" == "debug"
DEF     =-DDEBUG_RETAIL
CDEBUG  =$(DEF)
!else
DEF	=-DDEBUG
CDEBUG  =$(DEF)
!endif
!endif

!if "$(WIN32)" == "TRUE"
CFLAGS	=-DNT_THUNK32 -Oxt -D_X86_ -DWIN32 $(CDEBUG) -Fo$@
IS_32	=TRUE
WANT_C932=TRUE
OS	=i386
LB	=lib	# Don't want c816 lib
!else
CFLAGS	=-DNT_THUNK16 -GD -Oxwti -Asnw -DWIN16 $(CDEBUG) -Fo$@
IS_16	=TRUE
!endif

IS_OEM	=TRUE

!include $(ROOT)\build\project.mk

compman.obj:	..\..\$$(@B).c ..\..\compman.h ..\..\compmani.h ..\..\compddk.h ..\..\debug.h $(PINC)\icm.rc $(PINC)\vfw.h $(PINC)\avifmt.h $(PINC)\avifile.h $(PINC)\profile.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT $(NAME)
..\..\$(@B).c
<<
!endif

icm.obj:	..\..\icm.c ..\..\compman.h ..\..\compddk.h $(PINC)\icm.rc $(PINC)\vfw.h $(PINC)\avifmt.h $(PINC)\avifile.h $(PINC)\aviiface.h $(PINC)\msvideo.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT $(NAME)
..\..\$(@B).c
<<
!endif

thunk.obj:	..\..\thunk.c ..\..\compman.h ..\..\compmn16.h ..\..\compmani.h ..\..\thunk.h ..\..\debug.h
!if "$(WIN32)" != "TRUE"
	@$(CL) @<<
$(CFLAGS) -NT $(NAME)
..\..\$(@B).c
<<
!endif

$(PINC)\icm.rc:	..\..\$$(@F)
	@copy %s $@

$(PINC)\vfw.h $(PINC)\avifmt.h:	$(ROOT)\vfw.32\$$(@F)
	@copy %s $@

$(PINC)\avifile.h $(PINC)\aviiface.h:	$(ROOT)\avifile.32\$$(@F)
	@copy %s $@

$(PINC)\msvideo.h $(PINC)\profile.h:	$(ROOT)\msvideo.32\$$(@F)
	@copy %s $@
