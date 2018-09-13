!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\other\moniker=)
MODULE=moniker
INCLUDEDIRS=$(SRCDIR)\
    $(OTHER_INC_DIRS)\
    $(TOOLSINCLUDE)\
    $(TOOLSMISCINCLUDE)\
    $(ROOT)\

PCHPATH=$(BLDROOT)\otherinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\moniker.lib
LIBOBJS= \
         $(BLDDIR)\mkbase.obj  \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)



!endif		# NTMAKEENV
