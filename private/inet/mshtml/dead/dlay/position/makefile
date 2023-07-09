!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\position=)
MODULE=position
INCLUDEDIRS=$(DLAY_INC_DIRS) $(SRCDIR) $(ROOT)\src\dlay\dl
PCHPATH=$(BLDROOT)\dlayinc
PCHFILE=dlaypch

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\position.lib
LIBOBJS= $(BLDDIR)\position.obj

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)


!endif		# NTMAKEENV
