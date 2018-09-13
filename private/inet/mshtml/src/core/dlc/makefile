!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\dlc=)
MODULE=dlc
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS)
#PCHPATH=$(BLDROOT)\coreinc
#PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\dlc.lib
LIBOBJS=\
        $(BLDDIR)\typenav.obj\

all: $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
