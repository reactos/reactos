!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\site\types=)
MODULE=types
INCLUDEDIRS= $(SITE_INC_DIRS) $(SRCDIR) $(TYPEDIR) $(SRCROOT)\ctrl\types

!include $(ROOT)\src\f3\make\make1.inc

TYPETARG= \
        $(TYPEDIR)\anchor.hdl   \
        $(TYPEDIR)\area.hdl     \
        $(TYPEDIR)\basefont.hdl \
        $(TYPEDIR)\block.hdl    \
        $(TYPEDIR)\bmkcoll.hdl  \
        $(TYPEDIR)\bodyroot.hdl \
        $(TYPEDIR)\body.hdl     \
        $(TYPEDIR)\br.hdl       \
        $(TYPEDIR)\caption.hdl  \
        $(TYPEDIR)\collect.hdl  \
        $(TYPEDIR)\comment.hdl  \
        $(TYPEDIR)\csite.hdl    \
        $(TYPEDIR)\dd.hdl       \
        $(TYPEDIR)\div.hdl      \
        $(TYPEDIR)\dlist.hdl    \
        $(TYPEDIR)\document.hdl \
        $(TYPEDIR)\dt.hdl       \
        $(TYPEDIR)\e1d.hdl      \
        $(TYPEDIR)\eform.hdl    \
        $(TYPEDIR)\element.hdl  \
        $(TYPEDIR)\filter.hdl   \
        $(TYPEDIR)\frame.hdl    \
        $(TYPEDIR)\frameset.hdl \
        $(TYPEDIR)\frmsite.hdl  \
        $(TYPEDIR)\font.hdl     \
        $(TYPEDIR)\header.hdl   \
        $(TYPEDIR)\hedelems.hdl \
        $(TYPEDIR)\hr.hdl       \
        $(TYPEDIR)\iframe.hdl   \
        $(TYPEDIR)\img.hdl      \
        $(TYPEDIR)\imgbase.hdl  \
        $(TYPEDIR)\inputbtn.hdl \
        $(TYPEDIR)\inputtxt.hdl \
        $(TYPEDIR)\label.hdl    \
        $(TYPEDIR)\li.hdl       \
        $(TYPEDIR)\link.hdl     \
        $(TYPEDIR)\list.hdl     \
        $(TYPEDIR)\location.hdl \
        $(TYPEDIR)\map.hdl      \
        $(TYPEDIR)\marquee.hdl  \
        $(TYPEDIR)\object.hdl   \
        $(TYPEDIR)\olesite.hdl  \
        $(TYPEDIR)\olist.hdl    \
        $(TYPEDIR)\omrect.hdl   \
        $(TYPEDIR)\option.hdl   \
        $(TYPEDIR)\para.hdl     \
        $(TYPEDIR)\phrase.hdl   \
        $(TYPEDIR)\pluginst.hdl \
        $(TYPEDIR)\range.hdl    \
        $(TYPEDIR)\script.hdl   \
        $(TYPEDIR)\selecobj.hdl \
        $(TYPEDIR)\select.hdl   \
        $(TYPEDIR)\siterang.hdl \
        $(TYPEDIR)\style.hdl    \
        $(TYPEDIR)\sheet.hdl    \
        $(TYPEDIR)\sheetcol.hdl \
        $(TYPEDIR)\table.hdl    \
        $(TYPEDIR)\textelem.hdl \
        $(TYPEDIR)\txtedit.hdl  \
        $(TYPEDIR)\ulist.hdl    \
        $(TYPEDIR)\unknown.hdl  \
        $(TYPEDIR)\history.hdl  \
        $(TYPEDIR)\window.hdl

IDLTARGS=$(SDKDIR)\ocmm.h $(BLDDIR)\ocmm_i.c $(BLDDIR)\ocmm_p.c $(BLDDIR)\ocmm_d.c

$(BLDDIR)\om.bat : om.sed
!if "$(_HTMLLISTING)" == "1"
    dir $(BLDDIR)\*.htm /B /O:N > $(TMPDIR)\om.bat
    $(SED) -f $(SRCDIR)\om.sed $(TMPDIR)\om.bat > $(BLDDIR)\om.bat
    cd $(BLDDIR)
    $(BLDDIR)\om.bat
    cd $(SRCDIR)
!endif

all: $(TYPETARG) $(BLDDIR)\om.bat \
        $(SDKDIR)\ocmm.h \
        $(BLDDIR)\ocmm_i.c \
        $(BLDDIR)\ocmm_p.c \
        $(BLDDIR)\ocmm_d.c \

depend:: \
        $(SDKDIR)\ocmm.h \

clean::
    $(DEL) $(SDKDIR)\ocmm.h

# -------------------------------------------------------------
#  MIDL
# -------------------------------------------------------------

PRINTMSG=
WARN0=
!ifdef SILENTMODE
PRINTMSG=$(ECHO) Building $@...
WARN0=/w0
!endif

$(BLDDIR)\ocmm.h $(BLDDIR)\ocmm_i.c $(BLDDIR)\ocmm_p.c $(BLDDIR)\ocmm_d.c : ocmm.idl
    SET INCLUDE=$(INCLUDEPATH)
    $(PRINTMSG)
    $(MIDL) /ms_ext /c_ext\
        -Zp8\
        -cpp_cmd $(CC)\
        -cpp_opt "-nologo /DMUNGETYPLIB=1 /DMIDL_COMPILER $(DEFINES) -E -Tc"\
        -no_warn\
        -char unsigned\
        -header $(BLDDIR)\ocmm.h\
        -proxy $(BLDDIR)\ocmm_p.c\
        -dlldata $(BLDDIR)\ocmm_d.c\
        -iid $(BLDDIR)\ocmm_i.c\
        ocmm.idl

$(SDKDIR)\ocmm.h : $(BLDDIR)\ocmm.h ocmm.sed
    $(SED) -f ocmm.sed <$(BLDDIR)\ocmm.h >$(SDKDIR)\ocmm.h

!include $(MAKE2_INC)


!endif        # NTMAKEENV
