SOURCES_USED=$(ROOT)\common.inc

CHECKED_ALT_DIR=1

NO_DEBUG_HOOK=1
XMV_PARSE=1
#VIEW_SERVICES=1
#IE5_ZOOM=1

CONDITIONAL_INCLUDES= \
    winwlm.h \
    rpcerr.h \
    rpcmac.h \
    macname1.h \
    macpub.h \
    macapi.h \
    macname2.h \
    macocidl.h \
    maccontrols.h \
    mainwin.h \
    w16toff.hxx \
    print.h \
    regmsg16.h \
    urlmki.h \
    help.h \
    pgstup16.hxx \
    page.h \
    icapexp.h \
    ccolorsync.h \
    folders.h \
    pixmap.h \
    pngfilt.ic \

COREINCLUDES=\
    $(ROOT)\src\core\include; \
    $(ROOT)\src\core\cdutil; \
    $(ROOT)\external\inc; \
    $(ROOT)\types\$(O); \
    $(BASEDIR)\private\windows\inc;

SITEINCLUDES=\
    $(ROOT)\src\site\include; \
    $(ROOT)\src\site\base; \
    $(ROOT)\src\site\builtin; \
    $(ROOT)\src\site\dbind; \
    $(ROOT)\src\site\display; \
    $(ROOT)\src\site\download; \
    $(ROOT)\src\site\layout; \
    $(ROOT)\src\site\miscelem; \
    $(ROOT)\src\site\miscsite; \
    $(ROOT)\src\site\ole; \
    $(ROOT)\src\site\print; \
    $(ROOT)\src\site\style; \
    $(ROOT)\src\site\table; \
    $(ROOT)\src\site\text; \
    $(ROOT)\src\site\util; \
    $(ROOT)\src\site\view; \
    $(ROOT)\src\site\acc; \
    $(ROOT)\src\other\include; \
    $(ROOT)\src\other\htmldlg; \
    $(ROOT)\src\dlay\dl; \
    $(ROOT)\src\core\include; \
    $(ROOT)\src\core\cdutil; \
    $(ROOT)\external\inc; \
    $(ROOT)\external\inc\msls; \
    $(ROOT)\types\$(O); \
    $(BASEDIR)\private\windows\inc;

!if !defined(WARNING_LEVEL)
WARNING_LEVEL=W4
!endif

MSC_WARNING_LEVEL=-$(WARNING_LEVEL) -WX
IA64_WARNING_LEVEL=-$(WARNING_LEVEL:W4=W3) -WX -D_M_IA64
AXP64_WARNING_LEVEL=-$(WARNING_LEVEL:W4=W3) -WX -D_M_AXP64

C_DEFINES = $(C_DEFINES) \
    /D_WCTYPE_INLINE_DEFINED \
    /DNASHVILLE \
    /DSTRICT \
    /DUNICODE \
    /D_UNICODE \
    /DOLEDBVER=0x0200 \

!if $(ALPHA)
USER_C_FLAGS=/QAieee1
!endif

!if !$(FREEBUILD)
USE_RTTI=1
MSC_OPTIMIZATION=/Od
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /zm /zff /zgf
LINKER_NOICF=1      # ICF is s-l-o-w
USE_DEBUGLIB=1
!if $(386) && !defined(UNIX)
USE_STACK_SPEW=1
!endif
!else
MSC_OPTIMIZATION=/Oxs
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /ox /oe /zm /zff /zgf
!endif

MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) /Ob1 /Oi

NOT_LEAN_AND_MEAN=1

USE_NOLIBS=1
UMTYPE=windows

# Use NT 4.0/Win95 functionality only

WIN32_IE_VERSION=0x0400
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
SUBSYSTEM_VERSION=4.00

!if $(386)
NO_NTDLL=1
!else
# USE_NTDLL=1
!endif

CHECKED_ALT_DIR=1

# By default, don't generate a .bsc file
NO_BROWSER_FILE=1

!ifdef QUILL
C_DEFINES = $(C_DEFINES) /DQUILL
!endif

!ifdef TREE_SYNC
C_DEFINES = $(C_DEFINES) /DTREE_SYNC
!endif

!ifdef IE5_ZOOM
C_DEFINES = $(C_DEFINES) /DIE5_ZOOM
!endif

!ifdef IE6_ROTATE
C_DEFINES = $(C_DEFINES) /DIE6_ROTATE
!endif

!ifdef VIEW_SERVICES
C_DEFINES = $(C_DEFINES) /DVIEW_SERVICES
!endif

!ifdef XMV_PARSE
C_DEFINES = $(C_DEFINES) /DXMV_PARSE
!endif

!ifdef DISABLE_MEASURING
C_DEFINES = $(C_DEFINES) /DDISABLE_MEASURING
!endif

!ifdef USE_ICECAP
C_DEFINES = $(C_DEFINES) /DPRODUCT_PROF
!endif

!ifdef USE_PERFTAGS
!if $(FREEBUILD)
C_DEFINES = $(C_DEFINES) /DPERFTAGS
!message PERFTAGS was defined
USE_DEBUGLIB=1
!endif
!endif

!if !$(FREEBUILD) && !defined(UNIX)
C_DEFINES = $(C_DEFINES) /DPERFMETER
!message PERFMETER was defined
USE_DEBUGLIB=1
!endif

!ifdef USE_STACK_SPEW
C_DEFINES = $(C_DEFINES) /DUSE_STACK_SPEW
!message USE_STACK_SPEW was defined
!endif

!if "$(BUILD_ALT_DIR)"=="m" && "$(USE_PERFTAGS_IN_METER_BUILD)"=="1"
C_DEFINES = $(C_DEFINES:PERFMETER=PERFMETER /DPERFTAGS)
!message PERFTAGS was defined in METER build
USE_DEBUGLIB=1
!endif

!if "$(BUILD_ALT_DIR)"=="m"
USE_DEBUGLIB=1
!endif

!if $(FREEBUILD) && "$(BUILD_ALT_DIR)"!="m" && "$(USE_SWITCHES_IN_RETAIL_BUILD)"=="1"
C_DEFINES = $(C_DEFINES) /DUSESWITCHES
!message USESWITCHES was defined in RETAIL build
!endif

!if !$(FREEBUILD) || "$(OBJCNTCHK)"=="1"
C_DEFINES = $(C_DEFINES) /DOBJCNTCHK
!message OBJCNTCHK was defined
!endif

CRTMAIN=_DllMainStartup

!if defined(USE_STACK_SPEW) && defined(USE_DEBUGLIB)
CRTMAIN=_DllMainStartupDebugChkStk
!endif

!if defined(USE_STACK_SPEW) && !defined(USE_DEBUGLIB)
CRTMAIN=_DllMainStartupChkStk
!endif

!if !defined(USE_STACK_SPEW) && defined(USE_DEBUGLIB)
CRTMAIN=_DllMainStartupDebug
!endif

!if $(AXP64) || $(IA64)
# Turn off link warnings for AXP64 and IA64 links.  IEBUILD changes this to both from windbg.
NTDEBUGTYPE=$(NTDEBUGTYPE:both=windbg)
!endif

!if $(IA64)
# We want to allow -WX to work on IA64 builds
ALLOW_WX=1
!endif

!ifdef USE_MSHTML_INCREMENTAL_LINKING
!if !$(FREEBUILD) && $(USE_MSHTML_INCREMENTAL_LINKING)
NTDEBUG=ntsd
NTDEBUGTYPE=windbg
NTDBGFILES=1
!undef USE_MAPSYM
USE_INCREMENTAL_LINKING=1
LINKER_FLAGS=$(LINKER_FLAGS) -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
TARGETPDB=MSHTML
!message USE_MSHTML_INCREMENTAL_LINKING was defined
!message $(TARGETPDB) was defined
!endif
!else
# Compatibility with existing usage before use_mshtml_incremental_linking
!ifdef USE_INCREMENTAL_LINKING
!message DOING IT THE OLD WAY
LINKER_FLAGS = -INCREMENTAL:YES
USE_PDB_TO_COMPILE=1
!else
USE_MAPSYM=1
!endif
!endif

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif
