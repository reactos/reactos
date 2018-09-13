!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\other\htmldlg=)
MODULE=htmldlg
INCLUDEDIRS=$(SRCDIR)\
    $(OTHER_INC_DIRS)\
    $(TOOLSINCLUDE)\
    $(TOOLSMISCINCLUDE)\
    $(ROOT)\
    $(SRCROOT)\other\proppage\
    $(SRCROOT)\other\moniker\
    $(SRCROOT)\site\style\
    $(SRCROOT)\site\include\
    $(SRCROOT)\site\ole\
    $(SRCROOT)\site\util\

PCHPATH=$(BLDROOT)\otherinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

LIBTARG=$(BLDDIR)\scrtoid.lib
LIBOBJS= \
	$(BLDDIR)\peerhand.obj	\
	$(BLDDIR)\peerdisp.obj	\
	$(BLDDIR)\cssfilt.obj	\
	$(BLDDIR)\attrbag.obj
	

all: $(LIBTARG)

!include $(MAKE2_INC)


!endif		# NTMAKEENV
