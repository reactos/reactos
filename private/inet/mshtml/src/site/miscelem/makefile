!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\miscelem=)
MODULE=miscelem
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\util $(ROOT)
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\core\cdutil $(SRCROOT)\site\text
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\base $(SRCROOT)\other\include
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\builtin $(SRCROOT)\ctrl\include
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\2dsite $(SRCROOT)\site\style

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\miscelem.lib
LIBOBJS= \
         $(BLDDIR)\anchppg.obj  \
         $(BLDDIR)\div.obj      \
         $(BLDDIR)\eanchor.obj  \
         $(BLDDIR)\earea.obj    \
         $(BLDDIR)\eblock.obj   \
         $(BLDDIR)\ebr.obj      \
         $(BLDDIR)\edd.obj      \
         $(BLDDIR)\edlist.obj   \
         $(BLDDIR)\edt.obj      \
         $(BLDDIR)\efont.obj    \
         $(BLDDIR)\eform.obj    \
         $(BLDDIR)\eheader.obj  \
         $(BLDDIR)\elabel.obj   \
         $(BLDDIR)\eli.obj      \
         $(BLDDIR)\elist.obj    \
         $(BLDDIR)\emap.obj     \
         $(BLDDIR)\eolist.obj   \
         $(BLDDIR)\epara.obj    \
         $(BLDDIR)\ephrase.obj  \
         $(BLDDIR)\escript.obj  \
         $(BLDDIR)\estyle.obj   \
         $(BLDDIR)\eulist.obj   \
         $(BLDDIR)\hyplnk.obj   \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
