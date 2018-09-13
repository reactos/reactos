!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\miscsite=)
MODULE=miscsite
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\util $(ROOT)
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\core\cdutil $(SRCROOT)\site\text $(SRCROOT)\site\ole
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\base $(SRCROOT)\other\include $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\builtin $(SRCROOT)\ctrl\include

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\miscsite.lib
LIBOBJS= \
         $(BLDDIR)\ehr.obj      \
         $(BLDDIR)\frameset.obj \


all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
