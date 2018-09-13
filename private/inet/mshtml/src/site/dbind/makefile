!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\dbind=)
MODULE=dbind
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\ctrl\include $(SRCROOT)\site\text
INCLUDEDIRS=$(INCLUDEDIRS) $(ROOT)\src\ctrl\types $(SRCROOT)\site\ole
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\other\include $(SRCROOT)\other\types
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\builtin
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\dlay\dl
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\util $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\dlay\include

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\dbind.lib
LIBOBJS= \
         $(BLDDIR)\convert.obj \
         $(BLDDIR)\dbwalker.obj  \
         $(BLDDIR)\dbagent.obj  \
         $(BLDDIR)\dbinding.obj \
         $(BLDDIR)\detail.obj   \
         $(BLDDIR)\dtable.obj   \
         $(BLDDIR)\drecgen.obj  \
         $(BLDDIR)\detmap.obj \
         $(BLDDIR)\tscroll.obj \

all:  $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
