!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\style=)
MODULE=style
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\util $(SRCROOT)\site\text
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\style

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\style.lib

LIBOBJS=\
        $(BLDDIR)\style.obj \
        $(BLDDIR)\sprop.obj \
        $(BLDDIR)\sheets.obj
        $(BLDDIR)\sheetcol.obj


all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)


!endif          # NTMAKEENV
