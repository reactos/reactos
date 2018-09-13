!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\include=)
MODULE=dlayinc
INCLUDEDIRS=$(SDKDIR)\
            $(DLAY_INC_DIRS)\
            $(SRCDIR)\
            $(ROOT)\src\core\include\
            $(ROOT)\src\form\include\
            $(ROOT)\src\ctrl\include\

PCHPATH=$(BLDROOT)\dlayinc
PCHFILE=dlaypch

!include $(ROOT)\src\f3\make\make1.inc

all: $(PCHPATH)\$(PCHFILE).pch

!include $(MAKE2_INC)

!endif		# NTMAKEENV
