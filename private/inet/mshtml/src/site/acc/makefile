!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\acc=)
MODULE=acc
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base $(SRCROOT)\site\text

PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc


LIBTARG=$(BLDDIR)\accable.lib
LIBOBJS= \
    $(BLDDIR)\accbase.obj  \
    $(BLDDIR)\accelem.obj  \
    $(BLDDIR)\accwind.obj  \
    $(BLDDIR)\accutil.obj  \
    $(BLDDIR)\accbody.obj  \
    $(BLDDIR)\accanch.obj  \
    $(BLDDIR)\accarea.obj  \
    $(BLDDIR)\accbtn.obj   \
    $(BLDDIR)\accimg.obj   \
    $(BLDDIR)\acccheck.obj \
    $(BLDDIR)\accinimg.obj \
    $(BLDDIR)\accpswrd.obj \
    $(BLDDIR)\accedit.obj  \
    $(BLDDIR)\accradio.obj \
    $(BLDDIR)\accmarq.obj  \
    $(BLDDIR)\accframe.obj \
    $(BLDDIR)\accobj.obj   \
    $(BLDDIR)\acctable.obj \ 
    $(BLDDIR)\accsel.obj
    

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
