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

TYPETARG= \
        $(TYPEDIR)\dlgmodel.hdl \
        $(TYPEDIR)\htmldlg.hdl  \

LIBTARG=$(BLDDIR)\htmldlg.lib
LIBOBJS= \
         $(BLDDIR)\htmldlg.obj  \
         $(BLDDIR)\dlgframe.obj \
         $(BLDDIR)\dlgsite.obj  \
         $(BLDDIR)\dlgsink.obj  \
         $(BLDDIR)\dlgbind.obj  \
         $(BLDDIR)\dlgmodel.obj \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)


!endif		# NTMAKEENV
