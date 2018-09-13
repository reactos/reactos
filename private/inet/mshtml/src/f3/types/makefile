

ROOT=$(MAKEDIR:\src\f3\types=)
MODULE=f3types
INCLUDEDIRS=$(SRCDIR)\
    $(SRCROOT)\core\include\
    $(SRCROOT)\ctrl\include\
    $(SRCROOT)\other\include\
    $(SRCROOT)\f3\include\
    $(SRCROOT)\site\include\
    $(SRCROOT)\core\types\
    $(SRCROOT)\ctrl\types\
    $(SRCROOT)\other\types\
    $(SRCROOT)\site\types\
    $(SRCROOT)\f3\include\
    $(SRCROOT)\f3\types\
    $(TYPEDIR)\
    $(ROOT)\help\
    $(TOOLSINCLUDE)\
    $(TOOLSMISCINCLUDE)\
    $(SRCROOT)\..

!include $(ROOT)\src\f3\make\make1.inc

# -------------------------------------------------------------
# Targets built by "nmake all"
# -------------------------------------------------------------

all: \
    $(SDKDIR)\MSHTML.h      \
    $(SDKDIR)\MSHTMdid.h    \
    $(BLDDIR)\MSHTML.tlb    \

# -------------------------------------------------------------
#  DISPID header file
# -------------------------------------------------------------

DISPIDS=..\..\core\include\stddisp.h \
    ..\..\core\include\coredisp.h \
    ..\..\ctrl\include\ctrldisp.h \

$(SDKDIR)\MSHTMdid.h : $(DISPIDS) $(SRCROOT)\f3\types\dispid.sed
    $(SED) -f $(SRCROOT)\f3\types\dispid.sed $(DISPIDS) >$(SDKDIR)\MSHTMLdid.h

# -------------------------------------------------------------
#  Type library
# -------------------------------------------------------------

$(BLDDIR)\MSHTML.tlb $(SDKDIR)\MSHTML.h : forms3.odl
    SET INCLUDE=$(INCLUDEPATH)
    $(MIDL) /new /ms_ext /c_ext\
        /client none /server none\
    -cpp_cmd $(CC)\
    -cpp_opt "-nologo /DMIDL_COMPILER $(DEFINES) /D__MKTYPLIB__ -E -Tc"\
    -header $(TMPDIR)\MSHTML.hh\
    -iid $(BLDDIR)\f3_i.c\
    -tlb $(BLDDIR)\MSHTML.tlb\
    -out $(BLDDIR)\
    forms3.odl
    echo Calling SED...
    $(SED) -f $(SRCROOT)\f3\types\mkhtml.sed $(TMPDIR)\MSHTML.hh >$(TMPDIR)\MSHTML.h
    call comphtml $(TMPDIR)\MSHTML.h $(SRCDIR)\MSHTML.h




forms3.pp : forms3.odl
    SET INCLUDE=$(INCLUDEPATH)
    $(CC) -nologo /DMIDL_COMPILER /D__MKTYPLIB__ $(DEFINES) -E -Tc forms3.odl > $@

!include $(MAKE2_INC)

# -------------------------------------------------------------
# Targets built by "nmake depend"
# -------------------------------------------------------------

depend:: \
    $(SDKDIR)\MSHTML.h \
    $(BLDDIR)\MSHTML.tlb

depend::
    $(MKDEP) -n /I $(INCLUDEDIRS: = /I ) -s.h -P $(BLDDIR)\ forms3.idl > $(TMPDIR)\depend.tmp
    $(MKDEP) -n /I $(INCLUDEDIRS: = /I ) -s.tlb -P $(BLDDIR)\ forms3.odl >> $(TMPDIR)\depend.tmp
    $(MKDEP) -n /I $(INCLUDEDIRS: = /I ) -sxc.tlb -P $(BLDDIR)\ forms3.odl >> $(TMPDIR)\depend.tmp
    $(SED) -f << $(TMPDIR)\depend.tmp >> $(BLDDIR)\depend.inc
s/forms3.tlb/MSHTML.tlb/
<<$(KEEPFLAGS)
