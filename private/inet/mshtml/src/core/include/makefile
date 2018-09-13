!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\include=)
MODULE=coreinc
INCLUDEDIRS=$(CORE_INC_DIRS) $(SRCDIR)
PCHPATH=$(BLDROOT)\coreinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

all: $(PCHPATH)\$(PCHFILE).pch

!include $(MAKE2_INC)

clean::
    $(DEL) $(BINDIR)\MSHTML.pdb

!endif		# NTMAKEENV
