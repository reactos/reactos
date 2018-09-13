# -------------------------------------------------------------
# Symbols you can define in build.inc or your environment
# Note that all user definable symbols are prefixed with an "_".
#
# Most of these have reasonable defaults and it is not necessary to
#  define them in your environment. Only define them if the value needs
#  to be different than the default.
# -------------------------------------------------------------
# _BCHECKER             - If 1, a BoundsChecker Pro build is done.
#                           (You must have a licensed copy of BoundsChecker
#                            on your machine, plus set up some other files).
# _BROWSE               - full=2, partial=1, none=0 [default=0]
# _COPYMAC              - if 1, mfile Mac Forms3 build to Macintosh
# _DEBUG                - 0 = Retail with separate symbols
#                         1 = Debug
#                         2 = Retail with  symbols
#                         3 = Profile (retail w/ symbols + profile)
#                         4 = BBT (Lego)
# _KEEPTEMPFILES        - keep temporary makefiles [default=0]
# _MACFOLDER            - root Mac folder to which Mac Forms3 is mfile'd
# _MACSYMS              - compile and copy Macintosh symbol files
# _MACMSFORMS           - Prefix of Macintosh Forms3 files = "Microsoft Forms"
# _MACVERSION           - Postfix of Macintosh Forms3 files = "(2)"
# _MAC_FULLDEBUG        - Build debug version w/ debug office, ole, [default=0]
# _MAC_UTILDEBUG        - Build debug version of Mac DragDrop CFRG [default=0]
# _MAC_PCODE            - Build release using PCODE instead of native [default=1]
# _MAC_PROFILE_OFFICE   - Build release using profiled office code [default=0]
# _MACHINE              - [default=<current machine type>]
# _MAP                  - if 1 then build map files [default=0]
# _NO_INCREMENTAL_LINK  - 1 forces a full link. 0 implies _MAP=0
#                          [default=0 for _DEBUG==1, 1 otherwise]
# _NO_MINIMAL_REBUILD   - 0 = Allow C++ compiler to skip .CPP files if no class
#                             definitions in any .H files have changed
#                         1 = Normal file dependancies
#                          [default = 1]
# _NO_INCREMENTAL_COMP  - 0 = Allow C++ compiler to skip functions which have
#                             not changed since last compile
#                         1 = Normal complete compiles
#                          [default = 1]
# _PRODUCT              - product version [default=96P]
# _RELEASE_CANDIDATE    - build Release Canidate w/ correct VersionInfo [default=0]
# _STDODBC              - 1 = use StdOdbc files; 0 = don't
# _SYSTEM               - [default=winnt]
# _USERCFLAGS           - add to compiler flags [default=]
# _USERLFLAGS           - add to link flags [default=]
# _USERRFLAGS           - add to resource compiler flags [default=]
# _USERMFLAGS           - add to MIDL flags [default=]
# _USERDEFINES          - add to overall defines... [default=}
# _WARNINGLEVEL         - warning level [default=4 for debug, 3 for all else]
# _WARNINGSAREERRORS    - stop compiling on warnings [default=1]
# _HTMLLISTING          - combine all .htm in the types\ directory om.htm
#
#
# *******  Symbols given below should not be in your environment! ********
#
# -------------------------------------------------------------
# Symbols defined in makefile before including make1.inc
# -------------------------------------------------------------
# ROOT          - root of srctree  [required]
# MODULE        - name of module   [required]
# PCHFILE       - name of precompiled header, no extension or path [optional]
# PCHPATH       - path to precompiled header [optional]
# INCLUDEDIRS   - search for include files in these directories [optional]
# PDBFILE       - use this as pdb file, or if "NONE" don't use pdb file
#
# -------------------------------------------------------------
# Symbols defined in makefile after including make1.inc
# -------------------------------------------------------------
# LIBTARG       - build library
# LIBOBJS       - list of object files to add to library
# LIBOBJS_<PLAT>- where <PLAT> is a specific platform (ALPHA, INTEL, etc.)
#
# -------------------------------------------------------------
# Symbols defined by this file
# -------------------------------------------------------------
# BLDROOT       - root of build tree
# BLDDIR        - build directory for this module
# SRCROOT       - root of the source tree
# SRCDIR        - this directory
# TMPDIR        - location for temporary files
# BINDIR        - final products directory
# SDKDIR        - Forms3 SDK
# CRTLIBC       - path to LIBCMT[D].LIB (SHIP or _DEBUG, depends on flags)
# CRTLIBCD      - path to LIBCMTD.LIB (_DEBUG only)
# CRTLIBCP      - path to LIBCMT.LIB (PROFILE only)
# MACDLLCREATOR - Mac Creator Sig for Forms3 dll
# MACEXECREATOR - Mac Creator Sig for formtool.exe
# F3            - File prefix, FM20
#
# *************************************************************
#
# -------------------------------------------------------------
# Targets
# -------------------------------------------------------------
# beep          Beeps.  Generates a beep function that is audible to the
#                       developer.  Usage:  nmake all beep.
#
# all           Make everything in this module.
# clean         Delete build products from this module.
# depend        Make dependencies for this module.
# fresh         Make clean, depend and all.
#
# fall          Make everything in this module and all dependent modules.
# fclean        Delete build products from this module and all dep. modules.
# fdepend       Make dependencies for this module and all dependent modules.
# ffresh        Make clean, depend and all for this and all dependent modules.
#
# foo.cod       Build code listing for foo.cpp.
# foo.i         Build preprocessed file for foo.cpp.
# foo.obj       Build object file for foo.cpp.

!ifndef MODULE
!error MODULE must be defined in makefile
!endif

!ifndef ROOT
!error ROOT must be defined in makefile
!endif

SRCROOT=$(ROOT)\src

# _RELEASE is defined by the build subdirs, causing the build process
# to ignore the build.inc file.

!if "$(_RELEASE)" == ""
!if exist($(SRCROOT)\build.inc)
!include "$(SRCROOT)\build.inc"
!endif
!endif

!if exist($(MAKEDIR)\..\project.inc)
!include $(MAKEDIR)\..\project.inc
!endif

# -------------------------------------------------------------
# Set user variables to defaults
# -------------------------------------------------------------

!ifndef _DEBUG
_DEBUG=1
!endif

!ifndef _SYSTEM
_SYSTEM=winnt
!endif

!ifndef _MACHINE
_MACHINE=$(PROCESSOR_ARCHITECTURE)
!if "$(_MACHINE)" == ""
_MACHINE=x86
!endif
!endif

!ifndef _BROWSE
_BROWSE=0
!endif

!ifndef _MAP
_MAP=$(_DEBUG)
!endif

!ifndef _KEEPTEMPFILES
_KEEPTEMPFILES=0
!endif

!ifndef _WARNINGSAREERRORS
_WARNINGSAREERRORS=1
!endif

!ifndef _WARNINGLEVEL
!  if "$(_DEBUG)" == "1"
_WARNINGLEVEL=4
!  else
_WARNINGLEVEL=3
!  endif
!endif

!ifndef _PRODUCT
_PRODUCT=96P
!endif

!ifndef _RELEASE_CANDIDATE
_RELEASE_CANDIDATE=0
!endif

!ifndef _MACUNICODE
_MACUNICODE = 1
!endif

!ifndef _VIADUCT
_VIADUCT=1
!endif

!ifndef _BLDROOT

_BLDROOT=$(ROOT)\build

!if "$(_MACHINE)" == "MIPS"
_BLDROOT=$(_BLDROOT)\mips
!elseif "$(_MACHINE)" == "x86"
_BLDROOT=$(_BLDROOT)\win
!elseif "$(_MACHINE)" == "PPCMAC"
_BLDROOT=$(_BLDROOT)\ppcmac
!elseif "$(_MACHINE)" == "ALPHA"
_BLDROOT=$(_BLDROOT)\alpha
!elseif "$(_MACHINE)" == "PPC"
_BLDROOT=$(_BLDROOT)\ppc
!endif

!if "$(_DEBUG)" == "1"
_BLDROOT=$(_BLDROOT)\debug
!elseif "$(_DEBUG)" == "3"
_BLDROOT=$(_BLDROOT)\profile
!elseif "$(_DEBUG)" == "4"
_BLDROOT=$(_BLDROOT)\profdbg
!else
_BLDROOT=$(_BLDROOT)\ship
!endif

!if "$(_PRODUCT)" != "96P"
_BLDROOT=$(_BLDROOT).$(_PRODUCT)
!endif

!if "$(_BCHECKER)" == "1"
_BLDROOT=$(_BLDROOT).bc
!endif

!endif # ifndef _BLDROOT


NO_UNICODE_WRAPPERS = 1

!if "$(_MACHINE)" == "x86"
NO_UNICODE_WRAPPERS = 0
!elseif "$(_MACHINE)" == "PPCMAC"
!  if "$(_MACUNICODE)" == "1"
NO_UNICODE_WRAPPERS = 0
!  endif
!elseif "$(_MACHINE)" == "ALPHA"
# Linker bug prevents use of incremental linking for now (3/5/96)
_NO_INCREMENTAL_LINK=1
!endif



!ifndef _COPYMAC
#!if "$(_MACHINE)" == "PPCMAC" && "$(_MACFOLDER)" != ""
#_COPYMAC = 1
#!else
_COPYMAC = 0
#!endif
!endif

!ifndef _MAC_FULLDEBUG
_MAC_FULLDEBUG=0
!endif

!ifndef _MAC_UTILDEBUG
_MAC_UTILDEBUG=0
!endif

!ifndef _MAC_PCODE
_MAC_PCODE=1
!endif

!ifndef _MAC_PROFILE_OFFICE
_MAC_PROFILE_OFFICE=0
!endif

!ifndef _NO_INCREMENTAL_LINK
_NO_INCREMENTAL_LINK=0
!endif

!ifndef _NO_MINIMAL_REBUILD
_NO_MINIMAL_REBUILD=1
!endif

!ifndef _NO_INCREMENTAL_COMP
_NO_INCREMENTAL_COMP=1
!endif

!if "$(_DEBUG)" != "1"
_NO_INCREMENTAL_LINK=1
_NO_INCREMENTAL_COMP=1
!endif

!if "$(PDBFILE)" == "NONE"
_NO_MINIMAL_REBUILD=1
_NO_INCREMENTAL_COMP=1
!endif


!ifndef _HTMLLISTING
_HTMLLISTING=0
!endif

# -------------------------------------------------------------
# File prefix
# -------------------------------------------------------------

!ifndef _STDODBC
_STDODBC=0
!endif

# -------------------------------------------------------------
# Directories
# -------------------------------------------------------------

BLDROOT=$(_BLDROOT)
BLDDIR=$(BLDROOT)\$(MODULE)
TMPDIR=$(BLDROOT)
SRCDIR=$(MAKEDIR)
BINDIR=$(BLDROOT)\bin
SDKDIR=$(BLDROOT)\sdk
TYPEDIR=$(BLDROOT)\types
!ifndef TOOLSROOT
TOOLSROOT=$(ROOT)\tools
!endif
TOOLSDIR=$(TOOLSROOT)\$(_MACHINE)

!if "$(_MACHINE)" == "PPCMAC"
TOOLSUTILS=$(ROOT)\tools\x86\utils
!if "$(_DEBUG)" == "3"
TOOLSINCLUDE=$(TOOLSROOT)\macinc\vbcustom $(TOOLSROOT)\macinc\macole $(TOOLSROOT)\macinc $(TOOLSROOT)\macinc\macos $(TOOLSROOT)\macinc\profile $(TOOLSROOT)\include
!else
TOOLSINCLUDE=$(TOOLSROOT)\macinc\vbcustom $(TOOLSROOT)\macinc\macole $(TOOLSROOT)\macinc $(TOOLSROOT)\macinc\macos $(TOOLSROOT)\include
!endif
!else
TOOLSUTILS=$(TOOLSDIR)\utils
TOOLSINCLUDE=$(TOOLSROOT)\include
!endif
TOOLSMISCINCLUDE=$(ROOT)\external\inc

TOOLSBIN=$(TOOLSDIR)\bin
TOOLSLIB=$(TOOLSDIR)\lib
!if "$(_MACHINE)" != "PPCMAC"
CRTLIBC=$(TOOLSDIR)\lib\libcmt.lib
!else
CRTLIBC=$(TOOLSDIR)\lib\libc.lib
!endif
TOOLSLIBP=$(TOOLSDIR)\lib
!if ("$(_DEBUG)" == "1" && "$(_MACHINE)" != "PPCMAC") || ("$(_MACHINE)" == "PPCMAC" && "$(_MAC_FULLDEBUG)" == "1")
TOOLSBIND=$(TOOLSDIR)\bin.dbg
TOOLSLIBD=$(TOOLSDIR)\lib.dbg
#CRTLIBCD=$(TOOLSDIR)\lib.dbg\libcmtd.lib
CRTLIBCD=$(CRTLIBC)
!else
TOOLSBIND=$(TOOLSBIN)
TOOLSLIBD=$(TOOLSLIB)
!endif
!if "$(_DEBUG)" == "3"
TOOLSBINDP=$(TOOLSDIR)\bin.prf
TOOLSLIBP=$(TOOLSDIR)\lib.prf
TOOLSBIND=$(TOOLSDIR)\bin.prf
TOOLSLIBD=$(TOOLSDIR)\lib.prf
!if "$(_MACHINE)" != "PPCMAC"
CRTLIBCP=$(TOOLSLIBP)\libcmt.lib
!else
CRTLIBCP=$(CRTLIBC)
!endif
!else
TOOLSBINDP=$(TOOLSBIND)
!endif
MAKE2_INC=$(SRCROOT)\f3\make\make2.inc

# -------------------------------------------------------------
# Tools
# -------------------------------------------------------------

!if "$(PROCESSOR_ARCHITECTURE)" == ""
EAT_ERRORS=
DEL      = -DEL
DELALL   = -ECHO y | DEL
DELTREE  = DELTREE /y
XCOPY    = $(WINDIR)\COMMAND\XCOPY
!else
EAT_ERRORS=2>nul:
DEL      = -DEL $(EAT_ERRORS)
DELALL   = $(DEL) /Q
DELTREE  = $(DEL) /S /Q >nul:
XCOPY    = $(SYSTEMROOT)\SYSTEM32\XCOPY
!endif

SED      = $(TOOLSUTILS)\SED
MKDEP    = $(TOOLSUTILS)\MKDEP
TLMUNGE  = $(TOOLSUTILS)\TLMUNGE
STUFFODG = $(TOOLSUTILS)\STUFFODG
MKTYPLIB = $(TOOLSBIN)\MKTYPLIB
CC       = $(TOOLSBIN)\CL
BCC      = $(TOOLSBIN)\CL
LIBR     = $(TOOLSBIN)\LIB
LINK     = $(TOOLSBIN)\LINK
BSCMAKE  = $(TOOLSBIN)\BSCMAKE
MC       = $(TOOLSBIN)\MC
RC       = $(TOOLSBIN)\RC
PDLPARSE = $(BLDROOT)\PDLPARSE\PDLPARSE
ECHOTIME = $(TOOLSUTILS)\ECHOTIME
MAPSYM   = $(TOOLSBIN)\MAPSYM -nologo

!if "$(_DEBUG)" == "0" && "$(_MACHINE)" != "PPCMAC"
SPLIT    = $(TOOLSBIN)\SPLITSYM.EXE -a
!else
SPLIT    = REM No Split Symbol
!endif


!if "$(_MACHINE)" == "ALPHA"
AS       = $(TOOLSBIN)\asaxp /nopp
!elseif "$(_MACHINE)" == "PPC"
AS       = $(TOOLSBIN)\pas
!else
AS       = $(TOOLSBIN)\ML /c
!endif

!if "$(_BCHECKER)" != ""
!if "$(_NORMALCC)" == ""
BCC = \bchecker\BCOMPILE -Zop \bchecker.ini
!endif
LINK = $(TOOLSBIN)\BCLINK
!endif

RENAME   = RENAME
SET      = SET
ECHO     = ECHO
MKDIR    = MKDIR $(EAT_ERRORS)
COPY     = COPY
RMDIR    = RMDIR
DIR      = DIR

!if "$(_MACHINE)" == "PPCMAC"
MIDL     = $(ROOT)\tools\x86\bin\MIDL
MRC      = $(TOOLSBIN)\MRC
MFILE    = $(TOOLSBIN)\MFILE
MAP2SYM  = $(TOOLSBIN)\MAP2SYM
MAKEPEF  = $(TOOLSBIN)\MAKEPEF
MERGEPEF  = $(TOOLSUTILS)\MERGEPEF
!else
MIDL     = $(TOOLSBIN)\MIDL
!endif

# -------------------------------------------------------------
# Miscellaneous Symbols specific to Mac Forms3
# -------------------------------------------------------------
!if "$(TARGET)" == "maccopy"
_COPYMAC = 1
!endif



!if "$(_COPYMAC)" == "1"
!if "$(_MACVOLUME)" == ""
_MACVOLUME=:HD
!else
_MACVOLUME=:$(_MACVOLUME)
!endif
!if "$(_MACFOLDER)" == ""
_MACFOLDER=$(_MACVOLUME):Forms3
!endif

_MACMSFORMS=Microsoft Forms
_MACVERSION=(2)

!if "$(_MAC_FULLDEBUG)" == "1"
_MACFOLDER=$(_MACFOLDER):fulldebug
!else
!if "$(_DEBUG)" == "1"
_MACFOLDER=$(_MACFOLDER):debug
!elseif "$(_DEBUG)" == "3"
_MACFOLDER=$(_MACFOLDER):profile
!else
_MACFOLDER=$(_MACFOLDER):ship
!endif #"$(_DEBUG)" == "1"
!endif #"$(_MAC_FULLDEBUG)" == "1"

!endif #"$(_COPYMAC)" == "1"

MACDLLCREATOR = cfmg
MACEXECREATOR = FM20
MACSYMCREATOR = ????
MACTLBCREATOR = OLE2
MACDLLENTRY = -mac:init=WlmConnectionInit  -mac:term=WlmConnectionTerm


MACTARG =
PEFTARG =
# -------------------------------------------------------------
# Create output directories
# -------------------------------------------------------------

!if [$(MKDIR) $(_BLDROOT)]
!endif

!if [$(MKDIR) $(BLDDIR)]
!endif

!if [$(MKDIR) $(BINDIR)]
!endif

!if [$(MKDIR) $(TYPEDIR)]
!endif

# -------------------------------------------------------------
# Verbosity & Temp files
# -------------------------------------------------------------

NOLOGO=/nologo

# Use "!ifdef SILENTMODE" to determine if silent mode is on.

!if "$(MAKEFLAGS:S=)" != "$(MAKEFLAGS)"
SILENTMODE=1
!else
!undef SILENTMODE
!endif

!if $(_KEEPTEMPFILES)
KEEPFLAGS=KEEP
!else
KEEPFLAGS=NOKEEP
!endif

#
# Make sure the INCLUDE and CL environment variables are not set or that will
#  mess everything up!
#
!if [$(SET) INCLUDE=]
!endif
!if [$(SET) CL=]
!endif
!if [$(SET) ML=]
!endif

# -------------------------------------------------------------
# Rules
# -------------------------------------------------------------

# The suffixes list contains inferred dependent file extensions in
# order of precedence.  Do not add target file extensions to this list.
# The first .SUFFIXES line clears the default setttings.
.SUFFIXES:
.SUFFIXES: .cpp .cxx .c .hxx .h .asm .r .rc .h .pdl .hdl .idl

#
# .cpp -> .obj
#
{$(SRCDIR)}.cpp.obj:
        $(BCC) $(NOLOGO) $< $(PCHCXXFLAGS) @$(BLDDIR)\c.rsp

#
# .cpp -> .cod
#
.cpp.cod:
        $(BCC) $(NOLOGO) $< /Fc$@ $(PCHCXXFLAGS) @$(BLDDIR)\c.rsp

#
# .cpp -> .i
#
.cpp.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHCXXFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .cxx -> .obj
#
{$(SRCDIR)}.cxx.obj:
        $(BCC) $(NOLOGO) $< $(PCHCXXFLAGS) @$(BLDDIR)\c.rsp


#
# .cxx -> .cod
#
.cxx.cod:
        $(BCC) $(NOLOGO) $< /Fc$@ $(PCHCXXFLAGS) @$(BLDDIR)\c.rsp

#
# .cxx -> .i
#
.cxx.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHCXXFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .c -> .obj
#
{$(SRCDIR)}.c.obj:
    $(BCC) $(NOLOGO) $(PCHCFLAGS) $< @$(BLDDIR)\c.rsp

#
# .c -> .obj
# special case for the MIDL generated .c files
#
.c.obj:
    $(BCC) $(NOLOGO) $(PCHCFLAGS) $< @$(BLDDIR)\c.rsp

#
# .c -> .cod
#
.c.cod:
    $(BCC) $(NOLOGO) $(PCHCFLAGS) $< /Fc$@ @$(BLDDIR)\c.rsp

#
# .c -> .i
#
.c.i:
        $(BCC) $(NOLOGO) @<< >$@
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
/c
/E
$<
<<$(KEEFLAGS)

#
# .asm -> .obj
#
{$(SRCDIR)}.asm.obj:
        @rem Make nmake happy with this line
!if "$(_MACHINE)" == "ALPHA" || "$(_MACHINE)" == "PPCMAC"
# The alpha assembler can't always find the preprocessor (ugh!)
# The ppcmac assembler can't include regular header files (ugh!)
        $(CC) $(NOLOGO) /EP @$(BLDDIR)\c.rsp $< > $(BLDDIR)\$(<F:.asm=.i)
        $(AS) $(NOLOGO) $(AFLAGS) /Fo$@ $(BLDDIR)\$(<F:.asm=.i)
# The ppc assembler doesn't understand /nologo (ugh!)
!elseif "$(_MACHINE)" == "PPC"
        $(CC) $(NOLOGO) /EP @$(BLDDIR)\c.rsp $< > $(BLDDIR)\$(<F:.asm=.i)
        $(AS) -o$@ $(BLDDIR)\$(<F:.asm=.i)
!else
        $(ECHO) $(<F)
        $(AS) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
$(DEFINES: =
)
$(AFLAGS: =
)
/c
/Fo$@
$<
<<$(KEEFLAGS)
!endif

#
# .hxx -> .pch
#
{$(SRCDIR)}.hxx.pch:
        $(ECHO) #include "$<" > $(BLDDIR)\$(@B).cpp
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
/c
$(CFLAGS: =
)
/Yc$<
/Fo$(@R).obj
/Fp$(@R).pcT
$(BLDDIR)\$(@B).cpp
<<$(KEEPFLAGS)
        $(DEL) $(@R).pch
        $(RENAME) $(@R).pcT $(@B).pch

#
# .h -> .pch
#
{$(SRCDIR)}.h.pch:
        $(ECHO) #include "$<" > $(BLDDIR)\$(@B).c
        $(BCC) $(NOLOGO) @<<
/I$(INCLUDEPATH:;=
/I)
/c
$(CFLAGS: =
)
/Yc$<
/Fo$(@R).obj
/Fp$(@R).pcT
$(BLDDIR)\$(@B).c
<<$(KEEPFLAGS)
        $(DEL) $(@R).pch
        $(RENAME) $(@R).pcT $(@B).pch

#
# .rc -> .res
#
{$(SRCDIR)}.rc.res:
!ifdef SILENTMODE
        $(ECHO) $(<F)
!endif
        $(RC) /I$(RESINCLPATH:;= /I) $(RFLAGS) /fo$@ $<

#
# .rc -> .rsc
#
{$(SRCDIR)}.rc.rsc:
!ifdef SILENTMODE
        $(ECHO) $(<F)
!endif
        $(RC) /I$(RESINCLPATH:;= /I) $(RFLAGS) /M /fo$@ $<

#
# .odl -> .i
#
.odl.i:
        $(BCC) @<<
/I$(INCLUDEPATH:;=
/I)
/E
$<
>$@
<<$(KEEPFLAGS)

#
# .r -> .rsc
#
{$(SRCDIR)}.r.rsc:
        $(CC) $(NOLOGO) -EP /I$(RESINCLPATH:;= /I) -Tp $< $(DEFINES) > $(@R).r
        $(MRC) /I$(TOOLSROOT)\macinc\mrc $(MRFLAGS) /o$@ $(@R).r

#
# .pdl -> .hdl
#
{$(SRCDIR)}.pdl.hdl:
        $(CC) $(NOLOGO) @<< | $(PDLPARSE) <<
/I$(INCLUDEPATH:;=
/I)
$(CFLAGS: =
)
$(PCHCXXFLAGS: =
)
/c
/E
$<
<<$(KEEPFLAGS)
- $(TYPEDIR)\$(@B) $(@B).pdl $(TYPEDIR)
<<$(KEEPFLAGS)


#
# .idl -> .pp
#
.idl.pp:
    $(SET) INCLUDE=$(INCLUDEPATH)
    $(CC) /DMIDL_COMPILER $(DEFINES) -E -Tc $< >$(@B).pp


# -------------------------------------------------------------
# DEFINES
# -------------------------------------------------------------

!if "$(_DEBUG)" == "1"
DEFINES=/DDBG=1 /D_DEBUG
!elseif "$(_DEBUG)" == "3"
DEFINES=/DDBG=0 /DNDEBUG
!elseif "$(_DEBUG)" == "4"
DEFINES=/DDBG=1 /D_DEBUG
!else
DEFINES=/DDBG=0 /DNDEBUG
!endif

!if "$(_RELEASE_CANDIDATE)" == "1"
DEFINES=$(DEFINES) /D_RELEASE_BUILD
!endif

!if "$(_MAC_FULLDEBUG)" == "1"
DEFINES=$(DEFINES) /D_MAC_FULLDEBUG
!endif

DEFINES=$(DEFINES) $(_USERDEFINES)

CAPLIB=$(TOOLSLIB)\penter.lib

!if "$(_DEBUG)" == "3" || "$(_DEBUG)" == "4"
DEFINES=$(DEFINES) /DPRODUCT_PROF

!   if "$(_MACHINE)" == "PPCMAC"
CAPLIB=$(TOOLSLIBP)\mprof.lib
!   else
CAPLIB=$(TOOLSLIB)\icap.lib
!   endif

!endif

!if "$(_MACHINE)"=="x86"
DEFINES=$(DEFINES) /D_X86_=1
!elseif "$(_MACHINE)"=="MIPS"
DEFINES=$(DEFINES) /D_MIPS_=1
!else if "$(_MACHINE)" == "PPCMAC"
DEFINES=$(DEFINES) /D_MAC /D_MPPC_ /D__pascal= /D_WLM_NOFORCE_LIBS /D_MACOLENAMES
DEFINES=$(DEFINES) /DOLE2ANSI /D_WIN32NLS /D_WIN32REG /D_WIN32
DEFINES=$(DEFINES) /DOLDROUTINENAMES=0
DEFINES=$(DEFINES) /D_MACUNICODE /DUNICODE /D_UNICODE
!   if "$(_MAC_UTILDEBUG)" == "1"
DEFINES=$(DEFINES) /D_MAC_UTILDEBUG
!   endif
!else if "$(_MACHINE)" == "ALPHA"
DEFINES=$(DEFINES) /D_ALPHA_=1
!else if "$(_MACHINE)" == "PPC"
DEFINES=$(DEFINES) /D_PPC_=1
!else
!error Unknown machine $(_MACHINE)
!endif

!if "$(_VIADUCT)" == "1"
DEFINES=$(DEFINES) /DVIADUCT
!endif

DEFINES=$(DEFINES) /DPRODUCT_$(_PRODUCT) /DWIN32 /D_WINDOWS /DSTRICT

!if "$(_SYSTEM)" == "winnt"
DEFINES= $(DEFINES) /DUNICODE /D_UNICODE
!endif

!if "$(_NO_INCREMENTAL_LINK)" == "1"
DEFINES =$(DEFINES) /D_NO_INCREMENTAL_LINK
!endif

!if "$(_STDODBC)" == "1"
DEFINES=$(DEFINES) /D_STDODBC
!endif

!if "$(NO_UNICODE_WRAPPERS)" == "1"
DEFINES =$(DEFINES) /DNO_UNICODE_WRAPPERS
!endif

# -------------------------------------------------------------
# AFLAGS
# -------------------------------------------------------------

!if "$(PDBFILE)" == "NONE"
#   /Z7     generate debug info in OBJ file
AFLAGS =$(AFLAGS) -Z7
!else
#   /Zi     generate debug info in PDB file
AFLAGS =$(AFLAGS) -Zi
!endif

# -------------------------------------------------------------
# CFLAGS
# -------------------------------------------------------------

INCLUDEPATH=$(INCLUDEDIRS: =;)

!if "$(_MACHINE)" != "PPCMAC"
#   /Zp8    pack structs on 8 byte boundaries
#   /Oi     enable intrinsic functions
#   /MT     use Multi-Threaded Statically-linked C runtime library (LIBCMT.LIB)
#   /Zl     omit default library name in .OBJ
CFLAGS =$(NOLOGO) $(CFLAGS) -MT -Zp8 -Zl -Oi
!else if "$(_MACHINE)" == "PPCMAC"
#   /Oi     enable intrinsic functions
#   /QPb    order bitfields low to high
#   /Zp8    pack structures on 1-byte
#   /ML     Single-Threaded (default action)
CFLAGS = $(NOLOGO) $(CFLAGS) -ML -Oi -QPb -Zp8
!if "$(_MAC_PCODE)" == "1"
#   /Oq     generate pcode
#   /Gy     separate functions for linker required for opt:ref to work.
CFLAGS = $(CFLAGS) -Oq -Gy
!endif
!endif

!if "$(PDBFILE)" == "NONE"
#   /Z7     generate debug info in OBJ file
CFLAGS =$(CFLAGS) -Z7
!else
#   /Zi     generate debug info in PDB file
CFLAGS =$(CFLAGS) -Zi
!endif

!if "$(_MACHINE)" != "PPCMAC" && "$(_MACHINE)" != "PPC"
!if "$(_NO_MINIMAL_REBUILD)" == "0"
#   /Gm     enable minimal rebuild:  allow C++ compiler to skip .CPP files iff
#             no class definitions in any .H files have changed
CFLAGS =$(CFLAGS) -Gm
!endif

!if "$(_NO_INCREMENTAL_COMP)" == "0"
#   /Gi     enable incremental compile:  allow C++ compiler to skip functions
#             which have not changed since last compile
CFLAGS =$(CFLAGS) -Gi
!endif

#   /GB     "blended" optimizations
#   /Gz     __stdcall calling convention
CFLAGS =$(CFLAGS) -GB -Gz
!endif  # _MACHINE != "PPCMAC"

!if "$(_DEBUG)" == "1" || "$(_DEBUG)" == "4"
#   /Od     disable optimizations
#   /Ob1    inline expansion on (needed for unicode wrappers)
#   /Gy     separate functions for linker
#   /Gs     disable stack checking
#   /GR     enable run-time type information
CFLAGS =$(CFLAGS) -Od -Ob1 -Oi -Gy -Gs -GR
!else
#   /O1     minimize space, same as:
#           /Og  enable global optimization
#           /Os  favor code space
#           /Oy  enable frame pointer omission
#           /Ob1 inline expansion on
#           /Gf  enable string pooling
#           /Gy  separate functions for linker
#       /GF     enable string pooling in read-only memory
#       /Gy     separate functions for linker
#       /Gs     disable stack checking
CFLAGS =$(CFLAGS) -O1 -GF -Gy -Gs
!endif

!if "$(_DEBUG)" == "3" || "$(_DEBUG)" == "4"
#   /Gh     generate call to __penter in function calls
#           Used by cap and icecap for profiling.
CFLAGS =$(CFLAGS) /Gh
!endif

!ifdef PCHFILE
#   /Fp<file> name precompiled header file
#   /Yu[file] use .PCH file
PCHCXXFLAGS   = /Fp$(PCHPATH)\$(PCHFILE).pch /Yu$(PCHFILE).hxx
PCHCFLAGS   = /Fp$(PCHPATH)\$(PCHFILE).pch /Yu$(PCHFILE).h
PCHFILEOBJ = $(PCHPATH)\$(PCHFILE).obj
!else
PCHCXXFLAGS   =
PCHCFLAGS   =
PCHFILEOBJ =
!endif

#   /Fd[file] name .PDB file
!if "$(PDBFILE)" != "NONE"
!ifdef PDBFILE
PDBFILE=$(BINDIR)\MSHTM$(PDBFILE).pdb
CFLAGS =$(CFLAGS) /Fd$(PDBFILE)
!else
CFLAGS =$(CFLAGS) /Fd$(BINDIR)\MSHTML.pdb
!endif
!endif

!if "$(_BROWSE)" == "1"
#   /Fr[file] name source browser file
CFLAGS =$(CFLAGS) /Fr$(BLDDIR)^\
!endif

!if "$(_BROWSE)" == "2"
#   /FR[file] name extended .SBR file
CFLAGS =$(CFLAGS) /FR$(BLDDIR)^\
!endif

!if "$(_WARNINGSAREERRORS)" != "0"
#   /WX treat warnings as errors
CFLAGS = $(CFLAGS) /WX
!endif

CFLAGS=$(CFLAGS) /W$(_WARNINGLEVEL)
DEFINES=$(DEFINES) /DWARNINGLEVEL=$(_WARNINGLEVEL)

CFLAGS=$(CFLAGS) $(DEFINES) $(_USERCFLAGS)


# -------------------------------------------------------------
# LFLAGS
# -------------------------------------------------------------

!if "$(_MACHINE)"=="x86"
LINKMACHINE=iX86
!elseif "$(_MACHINE)" == "PPCMAC"
LINKMACHINE=MPPC
!else
LINKMACHINE=$(_MACHINE)
!endif

LFLAGS=$(NOLOGO) $(LFLAGS) /NODEFAULTLIB /machine:$(LINKMACHINE)

!if "$(_NO_INCREMENTAL_LINK)" != "1"
LFLAGS=$(LFLAGS) /incremental:yes
_MAP=0
!else
LFLAGS=$(LFLAGS) /incremental:no /release
!endif

!if "$(_MAP)" != "0"
LFLAGS=$(LFLAGS) /map
!endif

!if "$(_MACHINE)" != "PPCMAC"
!if "$(_DEBUG)" == "3" || "$(_DEBUG)" == "4"
LFLAGS=$(LFLAGS) /DEBUG:MAPPED /DEBUGTYPE:cv /opt:ref
!elseif "$(_DEBUG)" == "0"
LFLAGS=$(LFLAGS) /DEBUG /DEBUGTYPE:cv,fixup /opt:ref
!else
LFLAGS=$(LFLAGS) /DEBUG
!endif
!elseif "$(_DEBUG)" == "1"
LFLAGS=$(LFLAGS) /debug
!elseif "$(_MACHINE)" == "PPCMAC"
#BUGBUG: [TerryLu] PCODE generation must disable opt:ref to work.
#LFLAGS=$(LFLAGS) /opt:ref
!if "$(DEBUG)" != "0"
LFLAGS=$(LFLAGS) /debug
!endif
!endif

!ifdef PDBFILE
LFLAGS=$(LFLAGS) /PDB:$(PDBFILE)
!else
LFLAGS=$(LFLAGS) /PDB:$*.pdb
!endif

LFLAGS=$(LFLAGS) $(_USERLFLAGS)


# -------------------------------------------------------------
# RFLAGS
# -------------------------------------------------------------

!if "$(RESINCLUDEDIRS)" == ""
RESINCLUDEDIRS=$(INCLUDEDIRS)
!endif

RESINCLPATH=$(RESINCLUDEDIRS: =;)

RFLAGS=$(RFLAGS) $(DEFINES) $(_USERRFLAGS)

!if "$(_MACHINE)" == "PPCMAC"
RESINCLUDEDIRS= $(RESINCLUDEDIRS) $(TOOLSROOT)\macinc\mrc
MRFLAGS=$(DEFINES) $(_USERRFLAGS)
RFLAGS=$(RFLAGS) /m
!endif

# -------------------------------------------------------------
# Default Goal
# -------------------------------------------------------------

goal: all


