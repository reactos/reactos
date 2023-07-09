!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\wrappers=)
MODULE=wrappers
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS) $(SRCROOT)\fole2ui
PCHPATH=$(BLDROOT)\wrappers
PCHFILE=precomp

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\wrappers.lib
LIBOBJS=\
        $(BLDDIR)\comctl32.obj\
        $(BLDDIR)\dynwrap.obj\
        $(BLDDIR)\unicwrap.obj\
        $(BLDDIR)\winmm.obj \
        $(BLDDIR)\comdlg32.obj\
        $(BLDDIR)\shell32.obj\

LIBOBJS_PPCMAC=\
        $(BLDDIR)\MFmWrap.obj  \
        $(BLDDIR)\MStdWrap.obj  \

LIBOBJS_NOT96=\
        $(BLDDIR)\urlmon.obj \
        $(BLDDIR)\wininet.obj \
        $(BLDDIR)\hlink.obj \
        $(BLDDIR)\url.obj

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
