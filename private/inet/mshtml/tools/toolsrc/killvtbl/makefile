ROOT=$(MAKEDIR:\tools\toolsrc\killvtbl=)
MODULE=killvtbl
INCLUDEDIRS=$(SRCDIR)\
    $(SDKDIR)\
    $(TOOLSINCLUDE)\

PDBFILE=killvtbl

!include $(ROOT)\src\f3\make\make1.inc

EXETYPE=console
EXETARG=$(TOOLSUTILS)\killvtbl.exe
EXEOBJS=\
    $(BLDDIR)\killvtbl.obj

EXEXLIBS=$(CRTLIBC) $(TOOLSLIB)\kernel32.lib
  #  $(TOOLSLIB)\setargv.obj

all: $(EXETARG)

!include $(MAKE2_INC)

clean::
    $(DEL) $(PDBFILE)
