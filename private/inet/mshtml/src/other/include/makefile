!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\other\include=)
MODULE=otherinc
INCLUDEDIRS= $(OTHER_INC_DIRS) $(SRCDIR)
PCHPATH=$(BLDROOT)\otherinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

all: $(PCHPATH)\$(PCHFILE).pch

!include $(MAKE2_INC)

!endif		# NTMAKEENV
