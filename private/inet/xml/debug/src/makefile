!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\core\debug=)
MODULE=debug
PCHPATH=$(BLDROOT)\debug
PCHFILE=headers
PDBFILE=dbg
RESINCLUDEDIRS=$(TOOLSINCLUDE)\
        $(TOOLSMISCINCLUDE)\
        $(SRCROOT)\f3\include\
        $(SRCROOT)\core\include\
        $(SRCROOT)\..
!if "$(_MACHINE)"=="PPCMAC"
RESINCLUDEDIRS=$(RESINCLUDEDIRS)\
                $(TOOLSROOT)\macinc\mrc
!endif
INCLUDEDIRS=$(SRCDIR) $(CORE_INC_DIRS)

!include $(ROOT)\src\f3\make\make1.inc

# Always compile debug DLL with optimizations on.
CFLAGS=$(CFLAGS:-Od=)
CFLAGS=$(CFLAGS:-Ge=)
CFLAGS=$(CFLAGS) /O1

DLLTARG=$(BINDIR)\MSXMLdbg.dll
DLLDEF=$(BLDDIR)\f3debug.def

!if "$(_MACHINE)"=="x86"
DLLENTRY=_DllMainCRTStartup@12
!else
DLLENTRY=_DllMainCRTStartup
!endif

!if "$(_MACHINE)"=="PPCMAC"
MACTARG=$(BINDIR)\MSXMLdbg.trg
MACNAME=MXMLDbg.dll
!endif

FFAILFLAGS=$(CFLAGS:-Zi=)
FFAILFLAGS=$(FFAILFLAGS:-Gm=)

$(BLDDIR)\ffail.obj : ffail.cxx
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(FFAILFLAGS: =
)
/c
/Fo$@
ffail.cxx
<<$(KEEPFLAGS)

!if "$(_MACHINE)"=="PPCMAC"
MACRES= $(BLDDIR)\f3debug.rsc
!endif

DLLOBJS=\
        $(BLDDIR)\assert.obj\
        $(BLDDIR)\assertui.obj\
        $(BLDDIR)\dbgalloc.obj\
        $(BLDDIR)\debug.obj\
        $(BLDDIR)\debugui.obj\
        $(BLDDIR)\f3debug.res\
        $(BLDDIR)\ffail.obj\
        $(BLDDIR)\simfail.obj\
        $(BLDDIR)\iwatch.obj\
        $(BLDDIR)\allocspy.obj\
        $(BLDDIR)\view.obj\
        $(BLDROOT)\coremisc\objextid.obj\
        $(BLDDIR)\magic.obj\
        $(BLDDIR)\ws.obj\

DLLOBJS_PPCMAC=\
        $(BLDROOT)\cdutil\memutils.obj     \
        $(BLDROOT)\cdutil\MacOleSp.obj     \
!   if "$(_MACUNICODE)" == "1"
        $(BLDROOT)\cdutil\CStr.obj         \
        $(BLDROOT)\wrappers\unicwrap.obj   \
!   endif
        $(MACRES)

DLLXLIBS=\
        $(BLDROOT)\sdk\MXMLuid.lib \
        $(TOOLSLIB)\uuid.lib\
        $(CRTLIBC)\

DLLXLIBS_NOTPPCMAC=\
        $(TOOLSLIB)\gdi32.lib\
        $(TOOLSLIB)\kernel32.lib\
        $(TOOLSLIB)\user32.lib\
        $(TOOLSLIB)\advapi32.lib\
        $(TOOLSLIB)\ole32.lib\
        $(TOOLSLIB)\misc\oleaut32.lib\

DLLXLIBS_PPCMAC=\
!if "$(_MAC_PCODE)" == "1"
        $(TOOLSLIB)\pcode.lib            \
!endif
        $(TOOLSLIB)\interfac.lib         \
!if "$(_MAC_FULLDEBUG)" == "1"
        $(TOOLSLIBD)\ole2auto.lib        \
        $(TOOLSLIBD)\ole2d.lib           \
        $(TOOLSLIBD)\mso97d.lib          \
!else
        $(TOOLSLIB)\ole2auto.lib         \
        $(TOOLSLIB)\ole2.lib             \
!if "$(_DEBUG)" == "3" && "$(_MAC_PROFILE_OFFICE)" == "1"
        $(TOOLSLIBP)\mso97h.lib          \
!else
        $(TOOLSLIB)\mso97.lib            \
!endif
!endif


all :   $(PCHPATH)\$(PCHFILE).pch  $(DLLTARG) $(MACTARG)


$(DLLDEF) : f3debug.def
        $(CC) $(NOLOGO) -EP -Tcf3debug.def $(DEFINES) > $@


!include $(MAKE2_INC)

# -------------------------------------------------------------
#  Extra dependencies
# -------------------------------------------------------------

depend::
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.res -P $(BLDDIR)\ f3debug.rc >> $(BLDDIR)\depend.inc
!if "$(_MACHINE)"=="PPCMAC"
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.rsc -P $(BLDDIR)\ f3debug.r >> $(BLDDIR)\depend.inc
!endif

clean::
    $(DEL) $(PDBFILE)
    $(DEL) $(BLDDIR)\ffail.obj

maccopy::
    cd $(TOOLSBIN)
    $(MFILE) copy -n -c $(MACDLLCREATOR) -t shlb $(BINDIR)\MSXMLdbg.dll \
            "$(_MACFOLDER):MSXMLdbg.dll"
    cd $(ROOT)
    @echo. > $(MACTARG)

!endif		# NTMAKEENV
