!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\std=)
MODULE=std
INCLUDEDIRS=$(DLAY_INC_DIRS) $(SRCDIR)
PCHPATH=$(BLDROOT)\dlayinc
PCHFILE=dlaypch

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\std.lib
LIBOBJS= $(BLDDIR)\arrayi.obj

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif		# NTMAKEENV
