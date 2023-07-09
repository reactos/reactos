ROOT=$(MAKEDIR:\src\site\dfrm=)
MODULE=dfrm
INCLUDEDIRS=$(SRCDIR) $(SITE_INC_DIRS) $(ROOT)\src\form\include $(ROOT)\src\ctrl\include $(BLDROOT)\types $(ROOT)\src\dlay\dl $(ROOT)\src\site\base $(ROOT)\src\ctrl\types $(ROOT)\src\form\types $(ROOT)\src\core\cdutil $(ROOT)\src\site\2dsite
PCHPATH=$(BLDROOT)\coreinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

TYPETARG= \
        $(TYPEDIR)\basefrm.hdl \
        $(TYPEDIR)\datafrm.hdl \


LIBTARG=$(BLDDIR)\datadoc.lib
LIBOBJS= \
     $(BLDDIR)\basefrm.obj \
     $(BLDDIR)\datafrm.obj \
     $(BLDDIR)\repeater.obj \
     $(BLDDIR)\loadtxt.obj \
     $(BLDDIR)\matrix.obj \
     $(BLDDIR)\oledsite.obj \
     $(BLDDIR)\rsctrl.obj \
     $(BLDDIR)\rootfrm.obj \
     $(BLDDIR)\frmprops.obj \
     $(BLDDIR)\detail.obj \
     $(BLDDIR)\bitary.obj \
     $(BLDDIR)\ddselect.obj \
     $(BLDDIR)\datadoc.obj \
     $(BLDDIR)\propchg.obj \
     $(BLDDIR)\headfrm.obj\
     $(BLDDIR)\dflist.obj\


all: $(TYPETARG) $(LIBTARG) $(RESTARG)

!include $(MAKE2_INC)

