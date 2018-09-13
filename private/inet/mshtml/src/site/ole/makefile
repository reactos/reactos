!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\ole=)
MODULE=siteole
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(SRCROOT)\site\base $(SRCROOT)\dlay\dl
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\core\cdutil
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\util $(SRCROOT)\site\style
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\miscsite
INCLUDEDIRS=$(INCLUDEDIRS) $(SRCROOT)\site\text
PCHPATH=$(BLDROOT)\siteinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc


LIBTARG=$(BLDDIR)\siteole.lib
LIBOBJS= \
    $(BLDDIR)\eobject.obj  \
    $(BLDDIR)\frmsite.obj  \
    $(BLDDIR)\frame.obj    \
    $(BLDDIR)\iaccx.obj    \
    $(BLDDIR)\iframe.obj   \
    $(BLDDIR)\oleclnt.obj  \
    $(BLDDIR)\olecpc.obj   \
    $(BLDDIR)\oleobj.obj   \
    $(BLDDIR)\olesite.obj  \
    $(BLDDIR)\olexobj.obj  \
	 $(BLDDIR)\pluginst.obj \

all: $(TYPETARG) $(LIBTARG)

!include $(MAKE2_INC)

!endif		# NTMAKEENV
