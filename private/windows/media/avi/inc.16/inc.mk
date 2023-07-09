#############################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#
#	inc.mk
#
#	dev\sdk\inc16 include file dependency file
#
#	This file is include globally via the master.mk file.
#
#############################################################################

SDKINC16DIR = $(ROOT)\dev\sdk\inc16
SDKINCDIR   = $(ROOT)\dev\sdk\inc
DEVINC16DIR = $(ROOT)\dev\inc16
SDKINC16DEP = $(SDKINC16DIR)\windows.h \
              $(SDKINC16DIR)\windows.inc \
              $(SDKINC16DIR)\windowsx.h \
              $(SDKINC16DIR)\commctrl.h \
              $(SDKINC16DIR)\cpl.h \
              $(SDKINC16DIR)\imm.h \
              $(SDKINC16DIR)\prsht.h \
              $(SDKINC16DIR)\shell.h \
              $(SDKINC16DIR)\stress.h \
              $(SDKINC16DIR)\shellapi.h \
              $(SDKINC16DIR)\thunks.h \
              $(SDKINC16DIR)\toolhelp.inc \
              $(SDKINC16DIR)\toolhelp.h \
              $(SDKINC16DIR)\pif.h \
              $(SDKINC16DIR)\pif.inc \
              $(SDKINC16DIR)\avicap.h \
              $(SDKINC16DIR)\avifile.h \
              $(SDKINC16DIR)\avifmt.h \
              $(SDKINC16DIR)\aviiface.h \
              $(SDKINC16DIR)\compddk.h \
              $(SDKINC16DIR)\compman.h \
              $(SDKINC16DIR)\digitalv.h \
              $(SDKINC16DIR)\digitalv.rc \
              $(SDKINC16DIR)\dispdib.h \
              $(SDKINC16DIR)\drawdib.h \
              $(SDKINC16DIR)\mci.rc \
              $(SDKINC16DIR)\mciavi.h \
              $(SDKINC16DIR)\mciwnd.h \
              $(SDKINC16DIR)\mmreg.h \
              $(SDKINC16DIR)\msacm.h \
              $(SDKINC16DIR)\msacmdlg.dlg \
              $(SDKINC16DIR)\msviddrv.h \
              $(SDKINC16DIR)\msvideo.h \
              $(SDKINC16DIR)\netdi.h \
              $(SDKINC16DIR)\vcr.h \
              $(SDKINC16DIR)\vcr.rc \
              $(SDKINC16DIR)\vfw.h \
              $(SDKINC16DIR)\mmsystem.h \
              $(SDKINC16DIR)\wfext.h \
              $(SDKINC16DIR)\winnet.h \
              $(SDKINC16DIR)\winerror.h

INCDEP = $(INCDEP) $(SDKINC16DEP)

$(SDKINC16DIR)\commctrl.h: $(DEVINC16DIR)\commctrl.h
        mkpublic $(DEVINC16DIR)\commctrl.h $(SDKINC16DIR)\commctrl.h

$(SDKINC16DIR)\cpl.h: $(DEVINC16DIR)\cpl.h
        mkpublic $(DEVINC16DIR)\cpl.h $(SDKINC16DIR)\cpl.h

$(SDKINC16DIR)\imm.h: $(DEVINC16DIR)\imm.h
        mkpublic $(DEVINC16DIR)\imm.h $(SDKINC16DIR)\imm.h

$(SDKINC16DIR)\netdi.h: $(DEVINC16DIR)\netdi.h
        mkpublic $(DEVINC16DIR)\netdi.h $(SDKINC16DIR)\netdi.h

$(SDKINC16DIR)\prsht.h: $(DEVINC16DIR)\prsht.h
        mkpublic $(DEVINC16DIR)\prsht.h $(SDKINC16DIR)\prsht.h

$(SDKINC16DIR)\shell.h: $(DEVINC16DIR)\shell.h
        mkpublic $(DEVINC16DIR)\shell.h $(SDKINC16DIR)\shell.h

$(SDKINC16DIR)\shellapi.h: $(DEVINC16DIR)\shellapi.h
        mkpublic $(DEVINC16DIR)\shellapi.h $(SDKINC16DIR)\shellapi.h

$(SDKINC16DIR)\stress.h: $(DEVINC16DIR)\stress.h
        mkpublic $(DEVINC16DIR)\stress.h $(SDKINC16DIR)\stress.h

$(SDKINC16DIR)\windows.h: $(DEVINC16DIR)\windows.h
        mkpublic $(DEVINC16DIR)\windows.h $(SDKINC16DIR)\windows.h

$(SDKINC16DIR)\windows.inc: $(DEVINC16DIR)\windows.inc
        mkpublic $(DEVINC16DIR)\windows.inc $(SDKINC16DIR)\windows.inc

$(SDKINC16DIR)\windowsx.h: $(DEVINC16DIR)\windowsx.h
        mkpublic $(DEVINC16DIR)\windowsx.h $(SDKINC16DIR)\windowsx.h

$(SDKINC16DIR)\pif.inc: $(SDKINC16DIR)\pif.h

!IFNDEF IS_PRIVATE
$(DEVINC16DIR)\windows.inc: $(DEVINC16DIR)\windows.h
	rt $(DEVINC16DIR)\windows.inc
!ELSE
!IFNDEF IS_16
!IFNDEF WANT_16
$(DEVINC16DIR)\windows.inc: $(DEVINC16DIR)\windows.h
	rt $(DEVINC16DIR)\windows.inc
!ENDIF # IS_16
!ENDIF # WANT_16
!ENDIF # IS_PRIVATE

$(SDKINC16DIR)\pif.h: $(DEVINC16DIR)\pif.h
        mkpublic $(DEVINC16DIR)\pif.h $(SDKINC16DIR)\pif.h

$(SDKINC16DIR)\thunks.h: $(DEVINC16DIR)\thunks.h
        mkpublic $(DEVINC16DIR)\thunks.h $(SDKINC16DIR)\thunks.h

$(SDKINC16DIR)\toolhelp.inc: $(DEVINC16DIR)\toolhelp.inc
        mkpublic $(DEVINC16DIR)\toolhelp.inc $(SDKINC16DIR)\toolhelp.inc

$(SDKINC16DIR)\toolhelp.h: $(DEVINC16DIR)\toolhelp.h
        mkpublic $(DEVINC16DIR)\toolhelp.h $(SDKINC16DIR)\toolhelp.h

$(SDKINC16DIR)\avicap.h \
$(SDKINC16DIR)\avifile.h \
$(SDKINC16DIR)\avifmt.h \
$(SDKINC16DIR)\aviiface.h \
$(SDKINC16DIR)\compddk.h \
$(SDKINC16DIR)\compman.h \
$(SDKINC16DIR)\digitalv.h \
$(SDKINC16DIR)\digitalv.rc \
$(SDKINC16DIR)\dispdib.h \
$(SDKINC16DIR)\drawdib.h \
$(SDKINC16DIR)\mci.rc \
$(SDKINC16DIR)\mciavi.h \
$(SDKINC16DIR)\mciwnd.h \
$(SDKINC16DIR)\mmreg.h \
$(SDKINC16DIR)\msacm.h \
$(SDKINC16DIR)\msacmdlg.dlg \
$(SDKINC16DIR)\msviddrv.h \
$(SDKINC16DIR)\msvideo.h \
$(SDKINC16DIR)\vcr.h \
$(SDKINC16DIR)\vcr.rc \
$(SDKINC16DIR)\vfw.h: $(DEVINC16DIR)\$$(@F)
        mkpublic %s $@

$(SDKINC16DIR)\mmsystem.h: $(DEVINC16DIR)\$$(@F)
        mkpublic %s $@
        copy $(DEVINC16DIR)\mmsystem.inc $(@D)

$(SDKINC16DIR)\wfext.h: $(DEVINC16DIR)\wfext.h
        mkpublic $(DEVINC16DIR)\wfext.h $(SDKINC16DIR)\wfext.h

$(SDKINC16DIR)\winnet.h: $(DEVINC16DIR)\winnet.h
        mkpublic $(DEVINC16DIR)\winnet.h $(SDKINC16DIR)\winnet.h

$(SDKINC16DIR)\winerror.h: $(SDKINCDIR)\winerror.h
        mkpublic $(SDKINCDIR)\winerror.h $(SDKINC16DIR)\winerror.h
