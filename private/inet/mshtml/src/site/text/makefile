!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\text=)
MODULE=textsite
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\builtin $(SRCROOT)\core\cdutil
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\ctrl\include $(SRCROOT)ctrl\types
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\other\include $(SRCROOT)\other\types
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\2dsite $(SRCROOT)\site\miscelem
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\download $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\dbind $(SRCROOT)\site\ole


PCHPATH=$(BLDROOT)\textsite
PCHFILE=texthdrs

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\textsite.lib

LIBOBJS= \
         $(BLDDIR)\callmgr.obj  \
         $(BLDDIR)\clasifyc.obj \
         $(BLDDIR)\devdsc.obj   \
         $(BLDDIR)\disp.obj     \
         $(BLDDIR)\dobj.obj     \
         $(BLDDIR)\doc.obj      \
         $(BLDDIR)\dxfrobj.obj  \
         $(BLDDIR)\e1d.obj      \
         $(BLDDIR)\ebody.obj    \
         $(BLDDIR)\edit.obj     \
         $(BLDDIR)\edit2.obj    \
         $(BLDDIR)\erunptr.obj  \
         $(BLDDIR)\font.obj     \
         $(BLDDIR)\hash.obj     \
         $(BLDDIR)\ime.obj      \
         $(BLDDIR)\irange.obj   \
         $(BLDDIR)\ldte.obj     \
         $(BLDDIR)\line.obj     \
         $(BLDDIR)\linebrkr.obj \
         $(BLDDIR)\measure.obj  \
         $(BLDDIR)\nlsprocs.obj \
         $(BLDDIR)\notmgr.obj   \
         $(BLDDIR)\range.obj    \
         $(BLDDIR)\render.obj   \
         $(BLDDIR)\rtext.obj    \
         $(BLDDIR)\runptr.obj   \
         $(BLDDIR)\sift.obj     \
         $(BLDDIR)\text.obj     \
         $(BLDDIR)\txtelems.obj \
         $(BLDDIR)\txtparse.obj \
         $(BLDDIR)\txtsaver.obj \
         $(BLDDIR)\txtselr.obj  \
         $(BLDDIR)\txtsite.obj  \
         $(BLDDIR)\txtsrang.obj \
         $(BLDDIR)\txttrack.obj \
         $(BLDDIR)\util.obj     \
         $(BLDDIR)\uuid.obj     \
         $(BLDDIR)\uwrap.obj    \



all: $(TYPETARG) $(PCHPATH)\$(PCHFILE).pch $(LIBTARG) $(BLDDIR)\padline.obj

!include $(MAKE2_INC)

!endif		# NTMAKEENV
