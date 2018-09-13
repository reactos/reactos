!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\include=)
MODULE=siteinc
INCLUDEDIRS=$(SITE_INC_DIRS) $(SRCDIR)
PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

all: $(PCHPATH)\$(PCHFILE).pch

!include $(MAKE2_INC)


!endif		# NTMAKEENV
