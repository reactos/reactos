!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\encode=)
MODULE=encode
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(TOOLSINCLUDE) $(TOOLSINCLUDE)\misc $(SRCROOT)\core\include $(SRCROOT)\site\include
PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\encode.lib
LIBOBJS=$(BLDDIR)\cyrllic.obj\
        $(BLDDIR)\encode.obj\
        $(BLDDIR)\japanese.obj\
        $(BLDDIR)\schinese.obj\
        $(BLDDIR)\unicode.obj\

all:  $(LIBTARG)

!include $(MAKE2_INC)

!endif          # NTMAKEENV
