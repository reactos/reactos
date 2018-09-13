!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\builtin=)
MODULE=builtin
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\ctrl\include $(SRCROOT)\site\text
INCLUDEDIRS=$(INCLUDEDIRS) $(ROOT)\src\ctrl\types
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\other\include $(SRCROOT)\other\types
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\util $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\dlay\dl $(SRCROOT)\site\dbind
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\miscelem
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\core\cdutil

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\builtin.lib
LIBOBJS= \
         $(BLDDIR)\tcell.obj \
         $(BLDDIR)\trowcol.obj \
         $(BLDDIR)\tcalc.obj \
         $(BLDDIR)\table.obj \
         $(BLDDIR)\inputtxt.obj\
         $(BLDDIR)\inputbtn.obj\
         $(BLDDIR)\marquee.obj\
         $(BLDDIR)\eselect.obj\
         $(BLDDIR)\eoption.obj\
         $(BLDDIR)\imgbase.obj \
         $(BLDDIR)\imgelem.obj \
         $(BLDDIR)\btnhlper.obj \
         $(BLDDIR)\wselect.obj \

all:  $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
