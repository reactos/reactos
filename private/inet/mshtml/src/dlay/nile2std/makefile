!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\nile2std=)
MODULE=nile2std
INCLUDEDIRS=$(DLAY_INC_DIRS) $(SRCDIR) $(ROOT)\src\dlay\dl
PCHPATH=$(BLDROOT)\dlayinc
PCHFILE=dlaypch

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\nile2std.lib
LIBOBJS= \
!if "$(_DEBUG)" == "1"
         $(BLDDIR)\testnile.obj \
!endif
         $(BLDDIR)\change.obj   \
         $(BLDDIR)\coerce.obj   \
         $(BLDDIR)\colinfo.obj  \
         $(BLDDIR)\locate.obj   \
         $(BLDDIR)\newrow.obj   \
         $(BLDDIR)\rowset.obj   \
         $(BLDDIR)\scroll.obj   \
         $(BLDDIR)\seek.obj     \
         $(BLDDIR)\stdevent.obj \
         $(BLDDIR)\rowhndl.obj  \
         $(BLDDIR)\property.obj \
         $(BLDDIR)\async.obj \


all: $(LIBTARG) $(BLDDIR)\testnile.obj

!include $(MAKE2_INC)


!endif		# NTMAKEENV
