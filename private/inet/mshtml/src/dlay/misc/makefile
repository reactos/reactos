!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\misc=)
MODULE=dlaymisc
INCLUDEDIRS=. $(DLAY_INC_DIRS)
PDBFILE=NONE

!include $(ROOT)\src\f3\make\make1.inc

all: $(BLDDIR)\c.rsp \
	$(BLDDIR)\dlayidpr.obj

!include $(MAKE2_INC)

!endif		# NTMAKEENV
