!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\dlay\dl=)
MODULE=dl
INCLUDEDIRS=$(DLAY_INC_DIRS)\
            $(SRCDIR)\
			$(SRCROOT)\site\base\
			$(SRCROOT)\site\include\
			$(SRCROOT)\site\ole\
			$(SRCROOT)\site\util\
			$(SRCROOT)\site\style\

PCHPATH=$(BLDROOT)\dlayinc
PCHFILE=dlaypch

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\dl.lib
LIBOBJS= \
         $(BLDDIR)\binder.obj   \
         $(BLDDIR)\bookmark.obj \
         $(BLDDIR)\dbllink.obj  \
         $(BLDDIR)\dlcursor.obj \
         $(BLDDIR)\objcache.obj

all: $(PCHPATH)\$(PCHFILE).pch $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
