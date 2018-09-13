!if "$(_RELEASE)" == ""
!if exist($(SRCROOT)\build.inc)
!include "$(SRCROOT)\build.inc"
!endif
!endif

!ifndef _PRODUCT
_PRODUCT=96P
!endif

!ifndef ECHOTIME
!  if !defined(PROCESSOR_ARCHITECTURE)
ECHOTIME=$(SRCROOT)\..\tools\x86\utils\echotime
!  else
ECHOTIME=$(SRCROOT)\..\tools\$(PROCESSOR_ARCHITECTURE)\utils\echotime
!  endif
!endif

DOMAKE=$(MAKE) /nologo /$(MAKEFLAGS: =)
NOPATH=@SET PATH=
CD=CD

TALK=@$(ECHOTIME) ------ /H:M:S -------

!if "$(MAKEFLAGS:S=)" != "$(MAKEFLAGS)"
CD=@CD
DOMAKE=@$(DOMAKE)
!endif

!if "$(TARGET)" == "clean"
ACTION = Cleaning
!elseif "$(TARGET)" == "depend"
ACTION = Building dependencies in
!elseif "$(TARGET)" == "all"
ACTION = Building
!elseif "$(TARGET)" == "maccopy"
ACTION = Copying to Macintosh
!else
ACTION = Doing something in
!endif



# -------------------------------------------------------------
# F3
# -------------------------------------------------------------

# Build forms3 c-runtime library.

crt : coreinc
    -$(TALK) $(ACTION) $(SRCROOT)\f3\crt...
    $(CD) $(SRCROOT)\f3\crt
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Create header files from IDL and MC files in CORE, OTHER, SITE
types : corelibs sitelibs otherlibs dlaylibs
    -$(TALK) $(ACTION) $(SRCROOT)\f3\types...
    $(CD) $(SRCROOT)\f3\types
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Build public guid library.
# Uses public guids defined in COREMISC, OTHERMISC, SITEMISC, DLAYMISC
# and generated in TYPES by the midl compiler.

uuid : coremisc sitemisc othermisc dlaymisc types dlaytype types
    -$(TALK) $(ACTION) $(SRCROOT)\f3\uuid...
    $(CD) $(SRCROOT)\f3\uuid
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Build the resource DLL
!if "$(_MACHINE)" == "PPCMAC" && "$(_MAC_FULLDEBUG)" == "1"
rsrc : vbtypes
!else
rsrc : types
!endif
    -$(TALK) $(ACTION) $(SRCROOT)\f3\rsrc...
    $(CD) $(SRCROOT)\f3\rsrc
    $(NOPATH)
    $(DOMAKE) $(TARGET)

# Build debug dll.
debug : uuid
        -$(TALK) $(ACTION) $(SRCROOT)\core\debug...
        $(CD) $(SRCROOT)\core\debug
        $(NOPATH)
        $(DOMAKE) $(TARGET)

!if "$(_MACHINE)" == "PPCMAC"
MACUTIL = macutil
!else
MACUTIL =
!endif

# Build the form dll.
dll : corelibs sitelibs otherlibs dlaylibs crt uuid types rsrc debug $(MACUTIL)
    -$(TALK) $(ACTION) $(SRCROOT)\f3\dll...
    $(CD) $(SRCROOT)\f3\dll
    $(NOPATH)
    $(DOMAKE) $(TARGET)

!if "$(_MACHINE)" == "PPCMAC" && "$(_MAC_FULLDEBUG)" == "1"
vbtypes : types
    -$(TALK) $(ACTION) $(SRCROOT)\f3\oatemp...
    $(CD) $(SRCROOT)\f3\oatemp
    $(NOPATH)
    $(DOMAKE) $(TARGET)
!endif

# Build the BBT instrumented dll and BBcfg file
bbt : dll
!if "$(_DEBUG)" == "4"
    -$(TALK) $(ACTION) $(SRCROOT)\f3\bbt...
    $(CD) $(SRCROOT)\f3\bbt
    $(NOPATH)
    $(DOMAKE) $(TARGET)
!endif

pdlparse :
    -$(TALK) $(ACTION) $(SRCROOT)\f3\pdlparse...
    $(CD) $(SRCROOT)\f3\pdlparse
    $(NOPATH)
    $(DOMAKE) $(TARGET)

setup : dll
    -$(TALK) $(ACTION) $(SRCROOT)\f3\setup...
    $(CD) $(SRCROOT)\f3\setup
    $(NOPATH)
    $(DOMAKE) $(TARGET)

cxx :
    -$(TALK) $(ACTION) $(SRCROOT)\f3\cxx...
    $(CD) $(SRCROOT)\f3\cxx
    $(NOPATH)
    $(DOMAKE) $(TARGET)


htmlpad : dll debug
    -$(TALK) $(ACTION) $(SRCROOT)\f3\htmlpad...
    $(CD) $(SRCROOT)\f3\htmlpad
    $(NOPATH)
    $(DOMAKE) $(TARGET)


ROOTTARGS=pdlparse htmlpad

root : $(ROOTTARGS)


# -------------------------------------------------------------
# CORE
# -------------------------------------------------------------

coretype :
        -$(TALK) $(ACTION) $(SRCROOT)\core\types...
        $(CD) $(SRCROOT)\core\types
        $(NOPATH)
        $(DOMAKE) $(TARGET)

coremisc :
        -$(TALK) $(ACTION) $(SRCROOT)\core\misc...
        $(CD) $(SRCROOT)\core\misc
        $(NOPATH)
        $(DOMAKE) $(TARGET)

coreinc : coretype
        -$(TALK) $(ACTION) $(SRCROOT)\core\include...
        $(CD) $(SRCROOT)\core\include
        $(NOPATH)
        $(DOMAKE) $(TARGET)

cdbase: coreinc siteinc
        -$(TALK) $(ACTION) $(SRCROOT)\core\cdbase...
        $(CD) $(SRCROOT)\core\cdbase
        $(NOPATH)
        $(DOMAKE) $(TARGET)

cdutil : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\core\cdutil...
        $(CD) $(SRCROOT)\core\cdutil
        $(NOPATH)
        $(DOMAKE) $(TARGET)

dlc : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\core\dlc...
        $(CD) $(SRCROOT)\core\dlc
        $(NOPATH)
        $(DOMAKE) $(TARGET)

wrappers : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\core\wrappers...
        $(CD) $(SRCROOT)\core\wrappers
        $(NOPATH)
        $(DOMAKE) $(TARGET)

ctrlutil : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\ctrl\util...
        $(CD) $(SRCROOT)\ctrl\util
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build all libraries in CORE project.

CORELIBS= cdutil wrappers cdbase coremisc dlc ctrlutil

!if "$(_MACHINE)" == "PPCMAC"
macutil : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\core\macutil...
        $(CD) $(SRCROOT)\core\macutil
        $(NOPATH)
        $(DOMAKE) $(TARGET)

!endif

corelibs : $(CORELIBS)


# -------------------------------------------------------------
# OTHER
# -------------------------------------------------------------

# Builds public and private guids for OTHER.
othermisc :
        -$(TALK) $(ACTION) $(SRCROOT)\other\misc...
        $(CD) $(SRCROOT)\other\misc
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build OTHER precompiled header.
otherinc : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\other\include...
        $(CD) $(SRCROOT)\other\include
        $(NOPATH)
        $(DOMAKE) $(TARGET)

proppage : otherinc
        -$(TALK) $(ACTION) $(SRCROOT)\other\proppage...
        $(CD) $(SRCROOT)\other\proppage
        $(NOPATH)
        $(DOMAKE) $(TARGET)

htmldlg : otherinc moniker base
        -$(TALK) $(ACTION) $(SRCROOT)\other\htmldlg...
        $(CD) $(SRCROOT)\other\htmldlg
        $(NOPATH)
        $(DOMAKE) $(TARGET)

moniker : otherinc
        -$(TALK) $(ACTION) $(SRCROOT)\other\moniker...
        $(CD) $(SRCROOT)\other\moniker
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build all libraries in OTHER project.

OTHERLIBS = othermisc proppage htmldlg moniker


otherlibs : $(OTHERLIBS)


# -------------------------------------------------------------
# DLAY
# -------------------------------------------------------------

dlaymisc :
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\misc...
        $(CD) $(SRCROOT)\dlay\misc
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build STD header from IDL.
dlaytype : coreinc
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\types...
        $(CD) $(SRCROOT)\dlay\types
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build DLAY precompiled header.
dlayinc : dlaytype
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\include...
        $(CD) $(SRCROOT)\dlay\include
        $(NOPATH)
        $(DOMAKE) $(TARGET)

std : dlayinc
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\std...
        $(CD) $(SRCROOT)\dlay\std
        $(NOPATH)
        $(DOMAKE) $(TARGET)

dl : coreinc dlaytype
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\dl...
        $(CD) $(SRCROOT)\dlay\dl
        $(NOPATH)
        $(DOMAKE) $(TARGET)

!if "$(_STDODBC)" == "1"
stdodbc : std
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\stdodbc...
        $(CD) $(SRCROOT)\dlay\stdodbc
        $(NOPATH)
        $(DOMAKE) $(TARGET)
!endif

!if "$(_STDODBC)" == "1"
nile2std : stdodbc
!else
nile2std : std
!endif
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\nile2std...
        $(CD) $(SRCROOT)\dlay\nile2std
        $(NOPATH)
        $(DOMAKE) $(TARGET)

position : dlayinc
        -$(TALK) $(ACTION) $(SRCROOT)\dlay\position...
        $(CD) $(SRCROOT)\dlay\position
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build all libraries in dlay project.
dlaylibs : \
        dlaytype    \
        dlayinc     \
        std         \
        dl          \
!if "$(_STDODBC)" == "1"
        stdodbc     \
!endif
        position    \
        nile2std




# -------------------------------------------------------------
# SITE
# -------------------------------------------------------------

# Builds public and private guids for SITE.
sitemisc :
        -$(TALK) $(ACTION) $(SRCROOT)\site\misc...
        $(CD) $(SRCROOT)\site\misc
        $(NOPATH)
        $(DOMAKE) $(TARGET)

sitetype :
        -$(TALK) $(ACTION) $(SRCROOT)\site\types...
        $(CD) $(SRCROOT)\site\types
        $(NOPATH)
        $(DOMAKE) $(TARGET)

siteinc : coreinc sitetype
        -$(TALK) $(ACTION) $(SRCROOT)\site\include...
        $(CD) $(SRCROOT)\site\include
        $(NOPATH)
        $(DOMAKE) $(TARGET)

siteutil : siteinc
        -$(TALK) $(ACTION) $(SRCROOT)\site\util...
        $(CD) $(SRCROOT)\site\util
        $(NOPATH)
        $(DOMAKE) $(TARGET)

base: siteinc siteutil sitetype dlaytype
        -$(TALK) $(ACTION) $(SRCROOT)\site\base...
        $(CD) $(SRCROOT)\site\base
        $(NOPATH)
        $(DOMAKE) $(TARGET)

ole : base
        -$(TALK) $(ACTION) $(SRCROOT)\site\ole...
        $(CD) $(SRCROOT)\site\ole
        $(NOPATH)
        $(DOMAKE) $(TARGET)

2d : base
        -$(TALK) $(ACTION) $(SRCROOT)\site\2d...
        $(CD) $(SRCROOT)\site\2d
        $(NOPATH)
        $(DOMAKE) $(TARGET)

builtin : base siteutil ole
        -$(TALK) $(ACTION) $(SRCROOT)\site\builtin...
        $(CD) $(SRCROOT)\site\builtin
        $(NOPATH)
        $(DOMAKE) $(TARGET)

dbind : base builtin
        $(TALK) $(ACTION) $(SRCROOT)\site\dbind...
        $(CD) $(SRCROOT)\site\dbind
        $(NOPATH)
        $(DOMAKE) $(TARGET)

style : base
        $(TALK) $(ACTION) $(SRCROOT)\site\style...
        $(CD) $(SRCROOT)\site\style
        $(NOPATH)
        $(DOMAKE) $(TARGET)

text : base
        -$(TALK) $(ACTION) $(SRCROOT)\site\text...
        $(CD) $(SRCROOT)\site\text
        $(NOPATH)
        $(DOMAKE) $(TARGET)

miscelem: text
        $(TALK) $(ACTION) $(SRCROOT)\site\miscelem...
        $(CD) $(SRCROOT)\site\miscelem
        $(NOPATH)
        $(DOMAKE) $(TARGET)

miscsite: base
        $(TALK) $(ACTION) $(SRCROOT)\site\miscsite...
        $(CD) $(SRCROOT)\site\miscsite
        $(NOPATH)
        $(DOMAKE) $(TARGET)

2dsite : base
        -$(TALK) $(ACTION) $(SRCROOT)\site\2dsite...
        $(CD) $(SRCROOT)\site\2dsite
        $(NOPATH)
        $(DOMAKE) $(TARGET)

download : siteinc
        -$(TALK) $(ACTION) $(SRCROOT)\site\download...
        $(CD) $(SRCROOT)\site\download
        $(NOPATH)
        $(DOMAKE) $(TARGET)

jpglib : siteinc
        -$(TALK) $(ACTION) $(SRCROOT)\site\download\jpglib...
        $(CD) $(SRCROOT)\site\download\jpglib
        $(NOPATH)
        $(DOMAKE) $(TARGET)

encode : siteinc
        -$(TALK) $(ACTION) $(SRCROOT)\site\encode...
        $(CD) $(SRCROOT)\site\encode
        $(NOPATH)
        $(DOMAKE) $(TARGET)

display :
        -$(TALK) $(ACTION) $(SRCROOT)\site\display...
        $(CD) $(SRCROOT)\site\display\src
        $(NOPATH)
        $(DOMAKE) $(TARGET)

# Build all libraries in SITE project.

SITELIBS= sitemisc base style text builtin 2dsite siteutil encode download jpglib ole miscelem miscsite dbind display

sitelibs : $(SITELIBS)

