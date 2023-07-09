!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\download\jpglib=)
MODULE=jpglib
INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE)
PCHPATH=$(BLDROOT)\jpglib
PCHFILE=all

!include $(ROOT)\src\f3\make\make1.inc

_WARNINGLEVEL=3
_WARNINGSAREERRORS=1

LIBTARG=$(BLDDIR)\jpglib.lib
LIBOBJS=$(BLDDIR)\jcomapi.obj\
        $(BLDDIR)\jdapimin.obj\
        $(BLDDIR)\jdapistd.obj\
        $(BLDDIR)\jdcoefct.obj\
        $(BLDDIR)\jdcolor.obj\
        $(BLDDIR)\jddctmgr.obj\
        $(BLDDIR)\jdhuff.obj\
        $(BLDDIR)\jdinput.obj\
        $(BLDDIR)\jdmainct.obj\
        $(BLDDIR)\jdmarker.obj\
        $(BLDDIR)\jdmaster.obj\
        $(BLDDIR)\jdmerge.obj\
        $(BLDDIR)\jdphuff.obj\
        $(BLDDIR)\jdpostct.obj\
        $(BLDDIR)\jdsample.obj\
        $(BLDDIR)\jdtrans.obj\
        $(BLDDIR)\jidctflt.obj\
        $(BLDDIR)\jidctfst.obj\
        $(BLDDIR)\jidctint.obj\
        $(BLDDIR)\jidctred.obj\
        $(BLDDIR)\jmemmgr.obj\
        $(BLDDIR)\jmemnobs.obj\
        $(BLDDIR)\jquant1.obj\
        $(BLDDIR)\jquant2.obj\
        $(BLDDIR)\jutils.obj\

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
