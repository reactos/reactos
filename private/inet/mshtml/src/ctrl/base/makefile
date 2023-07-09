ROOT=$(MAKEDIR:\src\ctrl\base=)
MODULE=ctrlbase
INCLUDEDIRS=$(SRCDIR) $(CTRL_INC_DIRS) $(ROOT)\src\core\cdutil
PCHPATH=$(BLDROOT)\ctrlinc
PCHFILE=headers

!include $(ROOT)\src\f3\make\make1.inc

$(BLDROOT)\types\f3munge.obj : $(BLDROOT)\types\f3munge.cxx
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHFLAGS: =
)
/c
/Fo$@
$**
<<$(KEEPFLAGS)

f3munge.cod : $(BLDROOT)\types\f3munge.cxx
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHFLAGS: =
)
/c
/Fc$@
$**
<<$(KEEPFLAGS)

LIBTARG=$(BLDDIR)\ctrlbase.lib
LIBOBJS= $(BLDDIR)\basectrl.obj \
         $(BLDDIR)\control.obj  \
         $(BLDDIR)\propns.obj\
         $(BLDROOT)\types\f3munge.obj\

LIBOBJS_97=\
         $(BLDDIR)\basecf.obj   \
         $(BLDDIR)\clsdesc.obj  \
         $(BLDDIR)\wrapctrl.obj \
         $(BLDDIR)\wrapdv.obj   \
         $(BLDDIR)\wrapinpl.obj \
         $(BLDDIR)\wrapwnd.obj  \
         $(BLDDIR)\drawcap.obj  \
         $(BLDDIR)\stdprop.obj  \

all: $(LIBTARG)

!include $(MAKE2_INC)
