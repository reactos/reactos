
ROOT=$(MAKEDIR:\tools\toolsrc\tlmunge=)
MODULE=tlmunge
INCLUDEDIRS=$(SRCDIR)\
    $(TYPEDIR)\
    $(SDKDIR)\
    $(SRCROOT)\core\include\
    $(TOOLSINCLUDE)\
    $(TOOLSINCLUDE)\misc


PCHPATH=$(BLDROOT)\tlmunge
PCHFILE=headers

PDBFILE=munge

_DEBUG=0

!include $(ROOT)\src\f3\make\make1.inc

!if exist($(SRCDIR)\build.inc)
!include $(SRCDIR)\build.inc
!endif

!if "$(_USECAP)" != ""
DEFINES=$(DEFINES) /DSDK_CAP
!endif

EXETYPE=console
EXETARG=$(BINDIR)\tlmunge.exe
EXEOBJS=\
    $(BLDDIR)\tlmunge.obj\
!if "$(_MACHINE)"=="PPCMAC"
    $(BLDROOT)\cdutil\win2mac.obj   \
!endif

EXELIBS=\

!if "$(_MACHINE)"!="PPCMAC"
EXEXLIBS=\
!if "$(_DEBUG)" == "1"
        $(SDKDIR)\$(F3)dbg.lib\
!endif
        $(SDKDIR)\$(F3)uuid.lib\
        $(TOOLSLIB)\ole32.lib\
#    $(TOOLSLIB)\lego\msvcrt.lib\
    $(TOOLSLIB)\misc\oleaut32.lib\
    $(TOOLSLIB)\gdi32.lib\
    $(TOOLSLIB)\kernel32.lib\
    $(TOOLSLIB)\user32.lib\
    $(TOOLSLIB)\advapi32.lib\
    $(TOOLSLIB)\uuid.lib\
    $(CRTLIBC)\
!endif


all: $(PCHPATH)\$(PCHFILE).pch $(EXETARG)

!include $(MAKE2_INC)

clean::
    $(DEL) $(PDBFILE)
