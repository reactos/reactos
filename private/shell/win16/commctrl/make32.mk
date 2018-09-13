#
# 16-32 Common
#
COMMONMKFILE=make32.mk
ROOT = ..\..\..\..
NAME=commctrl
PRIVINC=ctlspriv

!ifdef VERDIR           # pass-2 stuff
ROOT=..\$(ROOT)
PCHOBJ0=commctrl.obj
PCHOBJ1=bmpload.obj  btnlist.obj  cutils.obj   draglist.obj
PCHOBJ2=header.obj   hotkey.obj menuhelp.obj progress.obj status.obj
PCHOBJ3=tbcust.obj   toolbar.obj  tooltips.obj trackbar.obj updown.obj
!endif

!ifndef WIN16
#
# 32-bit specific
#
MAKEDLL=1
DLLENTRY=LibMain

!ifdef VERDIR           # pass-2 stuff
PROJ = comctl32         # instead of $(NAME)
OBJS = $(PCHOBJ0) $(PCHOBJ1) $(PCHOBJ2) $(PCHOBJ3)
!endif

!include $(ROOT)\win\core\shell\shell32.mk

!ifdef VERDIR           # pass-2 stuff
!include ..\depend.mk

$(PROJ).res: $(NAME).res
        copy $(NAME).res $@

!endif

!else  ## WIN16
#
# 16-bit specific
#
BUILDDLL=TRUE
MKPUBLIC=TRUE

!ifdef VERDIR           # pass-2 stuff
CODESEG0 =_TEXT
MISCOBJ0=$(ROOT)\dev\sdk\lib16\libentry.obj
LIB0=MNOCRTDW LIBW MDLLCEW
!endif

!include $(ROOT)\win\core\shell\common.mk

!endif ## WIN16


