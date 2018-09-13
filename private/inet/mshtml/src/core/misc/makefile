!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\misc=)
MODULE=coremisc
INCLUDEDIRS=. $(CORE_INC_DIRS) $(TOOLSMISCINCLUDE) $(SRCROOT)\core\include
PDBFILE=NONE

!include $(ROOT)\src\f3\make\make1.inc

CFLAGS=$(CFLAGS)

all: \
	$(BLDDIR)\c.rsp \
    $(BLDDIR)\coreid.obj \
    $(BLDDIR)\coreidpr.obj\
    $(BLDDIR)\docobj.obj\
    $(BLDDIR)\dbuuid.obj \
    $(BLDDIR)\objextid.obj \

!include $(MAKE2_INC)

!endif		# NTMAKEENV
