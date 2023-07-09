!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\other\proppage=)
MODULE=proppage
INCLUDEDIRS=$(SRCDIR) $(OTHER_INC_DIRS)
PCHPATH=$(BLDROOT)\otherinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\proppage.lib
LIBOBJS= \
         $(BLDDIR)\commit.obj   \
         $(BLDDIR)\allpage.obj  \

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif		# NTMAKEENV
