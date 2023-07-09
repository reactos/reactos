!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\cdbase=)
MODULE=cdbase
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS) $(SRCROOT)\fole2ui $(SRCROOT)\core\cdutil $(SRCROOT)\form\include $(TOOLSINCLUDE) $(SRCROOT)\site\style $(SRCROOT)\site\base $(SRCROOT)\site\include $(SRCROOT)\site\text
PCHPATH=$(BLDROOT)\coreinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\cdbase.lib
LIBOBJS=\
        $(BLDDIR)\base.obj\
        $(BLDDIR)\baseerr.obj\
        $(BLDDIR)\baseprop.obj\
        $(BLDDIR)\basemso.obj\
        $(BLDDIR)\sclsdesc.obj\
        $(BLDDIR)\cnct.obj\
        $(BLDDIR)\connect.obj\
        $(BLDDIR)\sdocobj.obj\
        $(BLDDIR)\ipobj2.obj\
        $(BLDDIR)\ippaint.obj\
        $(BLDDIR)\olectrl.obj\
        $(BLDDIR)\recalchlp.obj \
        $(BLDDIR)\sctrl.obj\
        $(BLDDIR)\sdv.obj\
        $(BLDDIR)\sinpl.obj\
        $(BLDDIR)\undo.obj\
        $(BLDDIR)\verb.obj\
        $(BLDDIR)\iaccs.obj\
        $(BLDDIR)\prophelp.obj \

LIBOBJS_NOTPPCMAC=\
        $(BLDDIR)\cache.obj\


all: $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
