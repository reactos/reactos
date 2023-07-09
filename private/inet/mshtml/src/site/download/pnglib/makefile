ROOT=$(MAKEDIR:\src\site\download\pnglib=)
MODULE=pnglib
INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE) $(SRCROOT)\site\download\zlib
PCHPATH=$(BLDROOT)\pnglib
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

_WARNINGLEVEL=3
_WARNINGSAREERRORS=1

LIBTARG=$(BLDDIR)\pnglib.lib
LIBOBJS=$(BLDDIR)\png.obj\
        $(BLDDIR)\pngerror.obj\
        $(BLDDIR)\pngmem.obj\
        $(BLDDIR)\pngpread.obj\
        $(BLDDIR)\pngrcb.obj\
        $(BLDDIR)\pngread.obj\
        $(BLDDIR)\pngrtran.obj\
        $(BLDDIR)\pngrutil.obj\
        $(BLDDIR)\pngtrans.obj\
        $(BLDDIR)\pngio.obj\

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)
