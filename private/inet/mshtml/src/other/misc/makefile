!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\other\misc=)
MODULE=othrmisc
PDBFILE=NONE
INCLUDEDIRS=. $(OTHER_INC_DIRS) $(SRCDIR)

!include $(ROOT)\src\f3\make\make1.inc

all: \
	$(BLDDIR)\c.rsp \
	$(BLDDIR)\otherid.obj \
	$(BLDDIR)\othridpr.obj

!include $(MAKE2_INC)

!endif		# NTMAKEENV
