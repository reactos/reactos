!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def

.odl{$O\}.tlb:
    $(MIDL) /DDummy_EnvStuffing $(MIDL_MKTYPLIB_FLAGS) $<

!else
ROOT=$(MAKEDIR:\src\f3\htmlpad=)
MODULE=htmlpad
RESINCLUDEDIRS= $(TOOLSINCLUDE)\
    $(BLDDIR)\
    $(SRCROOT)\core\include\
    $(SRCROOT)\f3\include\
    $(SRCROOT)\..
INCLUDEDIRS=$(SRCDIR)\
    $(SDKDIR)\
    $(BLDDIR)\
    $(SRCROOT)\core\include\
    $(SRCROOT)\site\include\
    $(TOOLSINCLUDE)\
    $(TOOLSMISCINCLUDE)\
    $(ROOT)\
    $(BLDROOT)\types

PCHPATH=$(BLDROOT)\htmlpad
PCHFILE=headers
PDBFILE=pad

!include $(ROOT)\src\f3\make\make1.inc

EXETYPE=windows
EXETARG=$(BINDIR)\MSHTMpad.exe
EXEOBJS=\
        $(BLDDIR)\padole.obj \
		$(BLDDIR)\padsend.obj \
        $(BLDDIR)\msgtripl.obj \
        $(BLDDIR)\msgaddr.obj \
        $(BLDDIR)\msgreply.obj \
        $(BLDDIR)\cenum.obj \
        $(BLDDIR)\stdenum.obj \
        $(BLDDIR)\chsfld.obj \
        $(BLDDIR)\tvnode.obj \
        $(BLDDIR)\mapi.obj \
        $(BLDDIR)\msgguid.obj \
        $(BLDDIR)\msgdoc.obj \
        $(BLDDIR)\msghost.obj \
        $(BLDDIR)\msgwnd.obj \
        $(BLDDIR)\msgvwsnk.obj \
        $(BLDDIR)\msgerr.obj \
        $(BLDDIR)\msgcidx.obj \
        $(BLDDIR)\msgreg.obj \
        $(BLDDIR)\msgutil.obj \
        $(BLDDIR)\paddebug.obj \
	    $(BLDDIR)\padreg.obj\
        $(BLDDIR)\paddoc.obj \
        $(BLDDIR)\padsite.obj \
        $(BLDDIR)\padframe.obj \
        $(BLDDIR)\padmain.obj \
	    $(BLDDIR)\padfact.obj \
        $(BLDDIR)\padssite.obj \
        $(BLDDIR)\padbox.obj \
        $(BLDDIR)\padbsc.obj \
        $(BLDDIR)\padurl.obj \
        $(BLDDIR)\padauto.obj \
        $(BLDDIR)\padws.obj \
	    $(BLDDIR)\padguid.obj \
        $(BLDDIR)\padscrub.obj \
        $(BLDDIR)\pad.res \
        $(BLDDIR)\pad_i.obj\
        $(BLDROOT)\textsite\padline.obj \
        $(BLDROOT)\wrappers\unicwrap.obj \
        $(BLDROOT)\wrappers\urlmon.obj \
        $(BLDROOT)\wrappers\dynwrap.obj \
        $(BLDROOT)\coremisc\dbuuid.obj \
        $(BLDROOT)\coremisc\docobj.obj \
        $(BLDROOT)\coremisc\objextid.obj \
        $(BLDROOT)\sitemisc\siteidpr.obj\
        $(BLDROOT)\sitemisc\mshtmhst.obj \
        $(BLDROOT)\cdutil\memutils.obj \
	    $(BLDROOT)\cdutil\cputil.obj\
        $(BLDROOT)\crt\strutil.obj\
        $(BLDROOT)\cdutil\formsary.obj\
        $(BLDROOT)\cdutil\format.obj\
        $(BLDROOT)\cdutil\misccom.obj\
	    $(BLDROOT)\cdutil\cstr.obj\
        $(BLDROOT)\cdutil\regdbhlp.obj\
        $(BLDROOT)\wrappers\wininet.obj

#
# DO NOT ADD THESE LIBS: wininet.lib, urlmon.lib, mshtml.lib.  These must
# be dynamically loaded or you'll get mysterious memory leaks.
#

EXEXLIBS=\
    $(SDKDIR)\MSHTMuid.lib\
    $(TOOLSLIB)\ole32.lib\
    $(CRTLIBC)\
    $(TOOLSLIB)\misc\oleaut32.lib\
    $(TOOLSLIB)\gdi32.lib\
    $(TOOLSLIB)\kernel32.lib\
    $(TOOLSLIB)\user32.lib\
    $(TOOLSLIB)\advapi32.lib\
    $(TOOLSLIB)\uuid.lib\
    $(TOOLSLIB)\shell32.lib\
    $(TOOLSLIB)\comdlg32.lib\
    $(TOOLSLIB)\comctl32.lib\

$(BLDDIR)\pad_i.obj : $(BLDDIR)\pad_i.c
        $(BCC) /c /Fo$(BLDDIR)\pad_i.obj $(NOLOGO) $(CFLAGS) $(BLDDIR)\pad_i.c

$(BLDDIR)\pad.tlb $(SDKDIR)\f3pad.h : pad.odl
    SET INCLUDE=$(INCLUDEPATH)
    $(MIDL) -cpp_cmd $(CC) -cpp_opt "-nologo /DMIDL_COMPILER $(DEFINES) /D__MKTYPLIB__ -E -Tc"\
     /new\
     /tlb $(BLDDIR)\pad.tlb\
     /h $(BLDDIR)\f3pad.h\
     /iid $(BLDDIR)\pad_i.c\
     /I $(TOOLSMISCINCLUDE) /I $(SRCROOT)\core\include /win32 $(DEFINES) pad.odl

# Extra dependencies

depend:: $(BLDDIR)\pad.tlb

all: \
    $(BLDDIR)\pad.tlb \
    $(PCHPATH)\$(PCHFILE).pch \
    $(EXETARG) \

!include $(MAKE2_INC)

!endif		# NTMAKEENV
