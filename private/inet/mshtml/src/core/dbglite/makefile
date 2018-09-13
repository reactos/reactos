!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\dbglite=)
MODULE=dbglite
PDBFILE=NONE
INCLUDEDIRS=$(TOOLSINCLUDE)\
    $(CORE_INC_DIRS)\
    $(TOOLSMISCINCLUDE)\
    $(SRCROOT)\core\include\
    $(SRCROOT)\..

!include $(ROOT)\src\f3\make\make1.inc

CFLAGS=$(CFLAGS)

all: \
	$(BLDDIR)\dbglite.obj

!include $(MAKE2_INC)

!endif		# NTMAKEENV
