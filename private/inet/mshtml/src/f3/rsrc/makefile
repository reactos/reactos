!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
ROOT=$(MAKEDIR:\src\f3\rsrc=)
MODULE=rsrc

RESOURCE_ONLY=1

RESINCLUDEDIRS=$(TOOLSINCLUDE)\
        $(TOOLSMISCINCLUDE)\
        $(SRCROOT)\core\include\
        $(SRCROOT)\core\cdutil\
        $(SRCROOT)\other\include\
        $(SRCROOT)\ctrl\include\
        $(SRCROOT)\dlay\include\
        $(SRCROOT)\site\include\
        $(SRCROOT)\f3\include\
!if "$(_PRODUCT)" != "96"
        $(TYPEDIR)\
!endif
        $(SDKDIR)\
        $(SRCROOT)\..


!include $(ROOT)\src\f3\make\make1.inc

LFLAGS=$(LFLAGS) /BASE:0x70000000
DLLTARG=$(BINDIR)\MSHTMenu.dll


!if "$(_MACHINE)"=="PPCMAC"
MACTARG=$(BINDIR)\MSHTMenu.trg
MACRES= $(BLDDIR)\forms3r.rsc
MACNAME=$(_MACMSFORMS) enu Lib $(_MACVERSION)
!endif

DLLOBJS=\
        $(BLDDIR)\forms3r.res\

DLLOBJS_PPCMAC=\
        $(MACRES)\
#        $(TOOLSLIB)\comctl.rsc\
        $(TOOLSLIB)\ole2ui.rsc  # Do we need this for the resource DLL?

DLLXLIBS_PPCMAC=\
        $(CRTLIBC)\
!if "$(_MAC_PCODE)" == "1"
        $(TOOLSLIB)\pcode.lib\
!endif
        $(TOOLSLIB)\interfac.lib\
!if "$(_MAC_FULLDEBUG)" == "1"
        $(TOOLSLIBD)\mso97d.lib\
!else
!if "$(_DEBUG)" == "3" && "$(_MAC_PROFILE_OFFICE)" == "1"
        $(TOOLSLIBP)\mso97h.lib\
!else
        $(TOOLSLIB)\mso97.lib\
!endif
!endif


all : $(DLLTARG) $(MACTARG)


!include $(MAKE2_INC)

# -------------------------------------------------------------
#  Extra dependencies
# -------------------------------------------------------------

depend::
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.res -P $(BLDDIR)\ forms3r.rc >> $(BLDDIR)\depend.inc
    $(MKDEP) -n /I $(RESINCLUDEDIRS: = /I ) -s.rsc -P $(BLDDIR)\ forms3r.r >> $(BLDDIR)\depend.inc


maccopy::
    cd $(TOOLSBIN)
    $(MFILE) copy -n -c $(MACDLLCREATOR) -t shlb $(BINDIR)\MSHTMenu.dll \
            "$(_MACFOLDER):$(MACNAME)"
    cd $(ROOT)

!endif		# NTMAKEENV
