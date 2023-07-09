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
LANG    =usa

NAME	=compman
EXT	=lib
ROOT	=..
OBJS	=$(LOBJ)\compman.obj $(LOBJ)\icm.obj
GOALS	=$(PLIB)\$(NAME).$(EXT) $(PINC)\$(NAME).h $(PINC)\compddk.h $(PINC)\icm.rc

!if "$(NTDEBUG)" == ""
DEF	=
CDEBUG	=
ADEBUG	=
DEBUG   =RETAIL
LOBJ    =retail
!else
DEF	=-DDEBUG
CDEBUG	=-Zid $(DEF)
ADEBUG	=-Zi $(DEF)
DEBUG   =DEBUG
LOBJ    =debug
!endif

!if "$(STRICT)" == "YES"
TYPES	=-DSTRICT
!else
TYPES	=
!endif

CFLAGS	=-DWIN16 -DWIN31 -Asnw -Oxwt $(TYPES) $(CDEBUG) -I. -Fd$* -Fo$@
AFLAGS	=-D?MEDIUM -D?QUIET $(ADEBUG)

IS_16		=TRUE
IS_SDK		=TRUE
WANT_286        =TRUE
IS_PRIVATE      =FALSE  # Temporary until shlobj.h is added to sdk

!include $(ROOT)\bin.16\project.mk

$(LOBJ)\compman.obj:	$$(@B).c compman.h compddk.h ..\inc.16\icm.rc ..\inc.16\vfw.h ..\inc.16\avifmt.h ..\inc.16\avifile.h
	@$(CL) @<<
$(CFLAGS) -NT COMPMAN $(@B).c
<<

$(LOBJ)\icm.obj:	icm.c compman.h compddk.h ..\inc.16\icm.rc ..\inc.16\vfw.h ..\inc.16\avifmt.h ..\inc.16\avifile.h ..\inc.16\aviiface.h ..\inc.16\msvideo.h ..\inc.16\avicap.h
	@$(CL) @<<
$(CFLAGS) -NT ICMDLG $(@B).c
<<

#Weird kludge, copy the dialog to the inc directory for MSVIDEO inclusion
..\inc.16\icm.rc:        res\$(LANG)\$$(@F)
        @copy res\$(LANG)\$(@F) $@
        @copy res\$(LANG)\$(@F) $(PINC)

$(PINC)\vfw.h $(PINC)\avifmt.h:	..\inc.16\$$(@F)
	@copy ..\inc.16\$(@F) $@

$(PINC)\avifile.h $(PINC)\aviiface.h:	..\avifile.16\$$(@F)
	@copy ..\avifile.16\$(@F) $@

$(PINC)\msvideo.h:	..\msvideo.16\$$(@F)
	@copy ..\msvideo.16\$(@F) $@

$(PINC)\avicap.h:	..\avicap.16\$$(@F)
	@copy ..\avicap.16\$(@F) $@

$(PINC)\$(NAME).h: $(NAME).h
	@copy $(@F) $@

$(PINC)\compddk.h: compddk.h
	@copy $(@F) $@
