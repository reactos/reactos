!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\misc=)
MODULE=sitemisc
INCLUDEDIRS=. $(SITE_INC_DIRS) $(TOOLSINCLUDE)
PDBFILE=NONE

!include $(ROOT)\src\f3\make\make1.inc

CFLAGS=$(CFLAGS)

all: \
    $(BLDDIR)\c.rsp \
    $(BLDDIR)\siteid.obj \
    $(BLDDIR)\siteidpr.obj\
	$(BLDDIR)\mshtmhst.obj

!include $(MAKE2_INC)

!endif		# NTMAKEENV
