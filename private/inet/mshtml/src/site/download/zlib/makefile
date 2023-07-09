ROOT=$(MAKEDIR:\src\site\download\zlib=)
MODULE=zlib
INCLUDEDIRS=$(SRCDIR) $(TOOLSINCLUDE)
PCHPATH=$(BLDROOT)\zlib
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

_WARNINGLEVEL=3
_WARNINGSAREERRORS=1

LIBTARG=$(BLDDIR)\zlib.lib
LIBOBJS=$(BLDDIR)\infblock.obj\
        $(BLDDIR)\infutil.obj\
        $(BLDDIR)\uncompr.obj\
        $(BLDDIR)\adler32.obj\
        $(BLDDIR)\compress.obj\
        $(BLDDIR)\deflate.obj\
        $(BLDDIR)\infcodes.obj\
        $(BLDDIR)\inffast.obj\
        $(BLDDIR)\inflate.obj\
        $(BLDDIR)\inftrees.obj\
        $(BLDDIR)\trees.obj\
        $(BLDDIR)\zutil.obj\
        $(BLDDIR)\crc32.obj\

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)
