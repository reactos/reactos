!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\cdutil=)
MODULE=cdutil
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS)
PCHPATH=$(BLDROOT)\coreinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\cdutil.lib
LIBOBJS= \
         $(BLDDIR)\proppage.obj \
         $(BLDDIR)\atomtbl.obj  \
         $(BLDDIR)\bindtask.obj \
         $(BLDDIR)\brush.obj    \
         $(BLDDIR)\brushbmp.obj \
         $(BLDDIR)\buffer.obj \
         $(BLDDIR)\color.obj    \
         $(BLDDIR)\cputil.obj   \
         $(BLDDIR)\cenum.obj    \
         $(BLDDIR)\csimutil.obj \
         $(BLDDIR)\cstr.obj     \
         $(BLDDIR)\disputil.obj \
         $(BLDDIR)\disputl2.obj \
         $(BLDDIR)\dvutils.obj  \
         $(BLDDIR)\errinfo.obj  \
         $(BLDDIR)\errtbl.obj   \
         $(BLDDIR)\fatstg.obj   \
         $(BLDDIR)\fbstr.obj    \
         $(BLDDIR)\format.obj   \
         $(BLDDIR)\formsary.obj \
         $(BLDDIR)\help.obj     \
         $(BLDDIR)\himetric.obj \
         $(BLDDIR)\memutils.obj \
         $(BLDDIR)\misc.obj     \
         $(BLDDIR)\misccom.obj  \
         $(BLDDIR)\stmbuff.obj  \
         $(BLDDIR)\oeutil.obj   \
         $(BLDDIR)\offscrn.obj  \
         $(BLDDIR)\regdbhlp.obj \
         $(BLDDIR)\rotutils.obj \
         $(BLDDIR)\stdenum.obj  \
         $(BLDDIR)\stdfact.obj  \
         $(BLDDIR)\stgutils.obj \
         $(BLDDIR)\wutils.obj   \
         $(BLDDIR)\gwnd.obj     \
         $(BLDDIR)\tooltip.obj  \
         $(BLDDIR)\file.obj     \
         $(BLDDIR)\wndclass.obj \
         $(BLDDIR)\xform.obj    \
         //$(BLDDIR)\cdatadsp.obj \
         $(BLDDIR)\rostm.obj    \
         $(BLDDIR)\print.obj    \
         $(BLDDIR)\identpal.obj \
         $(BLDDIR)\buttutil.obj \
         $(BLDDIR)\color3d.obj  \
         $(BLDDIR)\border.obj   \
         $(BLDDIR)\genutil.obj

LIBOBJS_96=\
         $(BLDDIR)\unknown.obj  \
         $(BLDDIR)\dvtbl.obj    \
         $(BLDDIR)\toff.obj     \

LIBOBJS_NOT96=\
         $(BLDDIR)\specpage.obj \

LIBOBJS_96P=\
         $(BLDDIR)\taskman.obj  \
         $(BLDDIR)\tearoff.obj  \
         $(BLDDIR)\surface.obj  \

LIBOBJS_NOTPPCMAC=\
         $(BLDDIR)\memmg.obj    \

LIBOBJS_PPCMAC=\
         $(BLDDIR)\win2mac.obj  \
         $(BLDDIR)\macolesp.obj \
         $(BLDDIR)\macdraw.obj  \
         $(BLDDIR)\wlmdraw.obj  \
         $(BLDDIR)\ppcmac.obj   \

LIBOBJS_ALPHA=\
         $(BLDDIR)\alpha.obj \

LIBOBJS_PPC=\
         $(BLDDIR)\ppc.obj   \

all: $(LIBTARG)


!include $(MAKE2_INC)

!endif		# NTMAKEENV
