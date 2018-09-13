ROOT=$(MAKEDIR:\tools\toolsrc\stuffodg=)
MODULE=stuffodg
INCLUDEDIRS=$(SRCDIR)\
    $(SDKDIR)\
    $(SRCROOT)\core\include\
    $(TOOLSINCLUDE)\
    $(TOOLSINCLUDE)\misc

PCHPATH=$(BLDROOT)\stuffodg
PCHFILE=headers

PDBFILE=stuf

!include $(ROOT)\src\f3\make\make1.inc

!if "$(_USECAP)" != ""
DEFINES=$(DEFINES) /DSDK_CAP
!endif

EXETYPE=console
EXETARG=$(BINDIR)\stuffodg.exe
EXEOBJS=\
    $(BLDDIR)\stuffodg.obj\
!if "$(_MACHINE)"=="PPCMAC"
    $(BLDROOT)\cdutil\win2mac.obj   \
!endif

EXELIBS=\

!if "$(_MACHINE)"!="PPCMAC"
EXEXLIBS=\
!if "$(_DEBUG)" == "1"
    $(SDKDIR)\f3debug.lib\
!endif
    $(TOOLSLIB)\setargv.obj\
    $(TOOLSLIB)\ole32.lib\
    $(TOOLSLIB)\lego\msvcrt.lib\
    $(TOOLSLIB)\misc\oleaut32.lib\
    $(TOOLSLIB)\gdi32.lib\
    $(TOOLSLIB)\kernel32.lib\
    $(TOOLSLIB)\user32.lib\
    $(TOOLSLIB)\advapi32.lib\
    $(TOOLSLIB)\uuid.lib\
!endif


all: $(PCHPATH)\$(PCHFILE).pch $(EXETARG)

!include $(MAKE2_INC)

clean::
    $(DEL) $(PDBFILE)
