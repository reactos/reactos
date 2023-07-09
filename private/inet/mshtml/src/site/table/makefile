!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\table=)
MODULE=table
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\util $(ROOT)
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\core\cdutil $(SRCROOT)\site\text
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\base $(SRCROOT)\other\include
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\builtin $(SRCROOT)\ctrl\include
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\2dsite $(SRCROOT)\site\style

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\table.lib
LIBOBJS= \
         $(BLDDIR)\ltable.obj    \
         $(BLDDIR)\ltablekb.obj  \
         $(BLDDIR)\ltableom.obj  \
         $(BLDDIR)\ltcalc.obj    \
         $(BLDDIR)\ltdata.obj    \
         $(BLDDIR)\ltdraw.obj    \
         $(BLDDIR)\ltpos.obj     \
         $(BLDDIR)\ltcell.obj    \
         $(BLDDIR)\ltrowcol.obj  \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif          # NTMAKEENV
