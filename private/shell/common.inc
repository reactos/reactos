##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991-1998
#   All Rights Reserved.
#
##########################################################################

#
# Common include file 'sources' files in the shell project.
#
#
# These definitions are required in your sources file:
#
#     CCSHELL_DIR
#        Path to ccshell root.
#
#     FREEBUILD
#        Set by makefile.def file for you.  One of the following:
#          0      - build debug
#          1      - build retail
#
#
# Definitions used if defined:
#
#     TARGET_WIN95
#        Build component explicitly for the Win95 platform.  If this is
#        not defined, default is NT or both.
#
#     TARGET_BOTH
#        Build component to run on both platforms.
#
#     NO_PRIVATE_HEADERS
#        If set to 1, this is defined as a manifest constant, so the
#        stub headers in shell\inc\stubs do not include the private
#        companion headers.
#
#     BUILD_PRIVLIB
#        Build a private import library too.  See the comments in 
#        makefile.inc.
#
#     USE_NT_PRODUCT_VER
#        If defined, this will use the NT team's product version
#        numbers, as opposed to the IE team's numbers.
#
#
# Environment Variables and their meanings
#
#     ATL_DEBUG_QI
#        When set to 1, debug builds will also have _ATL_DEBUG_QI
#        defined.  ATL code spews a bunch of debug stuff during QueryInterface.
#
#     ATL_DEBUG_REFCOUNT
#        When set to 1, debug builds will also have _ATL_DEBUG_REFCOUNT
#        defined.  ATL code spews a bunch of debug stuff during AddRef
#        and Release.
#
#     FULL_DEBUG
#        If set to 1, then debug builds will also have FULL_DEBUG defined.
#        Retail builds unaffected.  This is highly encouraged for regular
#        development purposes.  Not to be set or used by the build labs.
#
#     MEASURE_PERF
#        If set to 1, enables performance timing code for both debug and
#        retail builds.  This code squirts the timings for interesting
#        events to the debug terminal.
#
#     USE_LEGO
#        Define lego macros
#

!if !defined(CCSHELL_DIR)
!error  CCSHELL_DIR must be defined in sources file
!endif

#
# Set private paths
#

INCLUDES        = \
    ..;\
    $(CCSHELL_DIR)\inc;\
    $(CCSHELL_DIR)\inc\stubs;\
    $(BASEDIR)\private\windows\inc;\
    $(SDK_INC_PATH);\
    $(BASEDIR)\private\inc;\
    $(INCLUDES)

#
# Skip these files when determining dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                  atlbase.h  \
                  atlcom.h   \
                  atlconv.h  \
                  atlctl.h   \
                  atlctl.cpp \
                  atliface.h \
                  atlimpl.cpp \
                  atlwin.h   \
                  atlwin.cpp \
                  ia64inst.h \
                  macapi.h   \
                  macname1.h \
                  macname2.h \
                  macocidl.h \
                  macpub.h   \
                  macwin32.h \
                  mainwin.h  \
                  mwversion.h \
                  penwin.h \
                  pshpck16.h \
                  rpcerr.h   \
                  rpcmac.h   \
                  setupx.h   \
                  skbapi.h   \
                  statreg.h  \
                  statreg.cpp \
                  unixstuff.h \
                  version.h  \
                  winwlm.h   \
                  ..\inc16\shellapi.h \
                  \vobs\userx\userx\public\sdk\inc\winbase.h    \
                  \vobs\userx\userx\public\sdk\inc\wingdi.h     \
                  \vobs\userx\userx\public\sdk\inc\winuser.h    \
                  \vobs\userx\userx\public\sdk\inc\shellapi.h   \
                  \vobs\userx\userx\public\sdk\inc\winspool.h   \
                  \vobs\userx\userx\public\sdk\inc\prsht.h      \
                  \vobs\userx\userx\public\sdk\inc\commdlg.h    \
                  \vobs\userx\userx\public\sdk\inc\shlobj.h     \
                  \vobs\userx\userx\public\sdk\inc\commctrl.h   \
                  \vobs\userx\userx\public\sdk\inc\shlguid.h    \
                  \vobs\userx\userx\public\sdk\inc\shlwapi.h    \
                  \vobs\userx\userx\public\sdk\inc\intshcut.h   \


!if !defined(TARGET_WIN95)

LIBRARY_PLATFORM_PATH   = $(BASEDIR)\public\sdk\lib

!else

# (use 'chicago' since that is already used in the NT tree)

INCLUDES                = $(INCLUDES);$(CCSHELL_DIR)\inc\win95;
LIBRARY_PLATFORM_PATH   = $(BASEDIR)\public\sdk\lib\chicago
ALT_PROJECT_TARGET      = chicago

!endif

#
# Set common options
#

# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
!if !defined(WIN32_WINNT_VERSION)
WIN32_WINNT_VERSION=0x0400
!endif
!if !defined(WIN32_WIN95_VERSION)
WIN32_WIN95_VERSION=0x0400
!endif
!if !defined(WIN32_IE_VERSION)
#   WARNING!  If you increment WIN32_IE_VERSION, make sure to tell the
#   Wx86 people or their build will break!
WIN32_IE_VERSION=0x0501
!endif

# Override NT5 settings
SUBSYSTEM_VERSION=4.00

# Defining this allows windows.h to include other headers
NOT_LEAN_AND_MEAN= 1

# Don't link to the runtime libs
USE_NOLIBS      = 1

# Don't link to NTDLL if this runs on both platforms
!ifdef TARGET_BOTH
NO_NTDLL=1
!endif

!if defined(TARGET_WIN95)
#  Don't define WINNT
CHICAGO_PRODUCT = 1
!endif

UMTYPE          = windows

# Ignore the following linker warnings
#    4049:locally defined symbol "Xxx" imported
LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049

#
# Lego options
#
!if defined(USE_LEGO)
NTBBT           = 1
NTPROFILEINPUT  = 1
!endif

#
# Additional compiler flags
#

C_DEFINES       = $(C_DEFINES) -DWIN32 -D_WIN32 -DPOSTSPLIT -DUSE_MIRRORING

C_DEFINES       = $(C_DEFINES) -DUNICODE_SHDOCVW

!if defined(TARGET_DEMO)
C_DEFINES       = $(C_DEFINES) -DWINNT_6DEMO
!endif

!ifdef USE_ICECAP
C_DEFINES = $(C_DEFINES) -DPRODUCT_PROF
!endif

!if !defined(TARGET_WIN95)
C_DEFINES       = $(C_DEFINES) -DNT

!  if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!  endif
!endif


# ------ Debug Only Defines --------

!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DDEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w

# Check for missing PURE directives in interface decls (debug only)
C_DEFINES       = $(C_DEFINES) -DNO_NOVTABLE

# Define FULL_DEBUG for asserts to break by default
!  if "$(FULL_DEBUG)" == "1"
C_DEFINES       = $(C_DEFINES) -DFULL_DEBUG 
!  endif

# Allow someone to have ATL_DEBUG_REFCOUNT and ATL_DEBUG_QI set 
# in their environment to turn this on
!  if "$(ATL_DEBUG_REFCOUNT)" == "1"
C_DEFINES       = $(C_DEFINES) -D_ATL_DEBUG_REFCOUNT
!  endif

!  if "$(ATL_DEBUG_QI)" == "1"
C_DEFINES       = $(C_DEFINES) -D_ATL_DEBUG_QI
!  endif

!endif
# ------ End Debug Only Defines --------


!if "$(MEASURE_PERF)" == "1"
C_DEFINES       = $(C_DEFINES) -DMEASURE_PERF
!endif

!if "$(NO_PRIVATE_HEADERS)" == "1"
C_DEFINES       = $(C_DEFINES) -DNO_PRIVATE_HEADERS
!endif

!ifdef USE_NT_PRODUCT_VER
# Use the NT product version numbers
C_DEFINES       = $(C_DEFINES) -UNASHVILLE
!else
# Use the IE product version numbers.  (The ntverp.h file checks
# for NASHVILLE.)
C_DEFINES       = $(C_DEFINES) -DNASHVILLE
!endif

# This define helps us distinguish our development environment
# BUGBUG (scotth): I think we can get rid of this now
C_DEFINES       = $(C_DEFINES) -DWINNT_ENV

# IE40-only features

FEATURE_IE40    = 1

!ifdef FEATURE_IE40
C_DEFINES       = $(C_DEFINES) -DFEATURE_IE40 -DNASH
!endif


# Make warnings equivalent to errors

!if !defined(MSC_WARNING_LEVEL)
MSC_WARNING_LEVEL= /W3
!endif
MSC_WARNING_LEVEL= $(MSC_WARNING_LEVEL) /WX

!if defined(TARGET_WIN95) || defined(TARGET_BOTH)
USE_MAPSYM      = 1
!endif


!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)


!ifdef BUILD_PRIVLIB
NTTARGETFILE1   = $(NTTARGETFILE1) $(TARGETPATHLIB)\*\$(PRIVLIB)
!endif


# Short: if you're a shell dev, you should have SHELLDEV=1 set in your 
# environment.
# 
# Long: Components like BROWSEUI.DLL build standard and alternate DLLs depending
# on the BUILD_PRODUCT environment variable.  (See browseui\sources.inc for
# more details.)  Because of other expectations in iedev\lib that we (the
# shell team) need, we must have our razzle environment set BUILD_PRODUCT=IE.
# However, in the scope of the shell project, we need to change this to 
# BUILD_PRODUCT=NT so we mimic the NT build lab's settings.  But the IE team 
# needs the BUILD_PRODUCT to remain "IE".  We key off of SHELLDEV to know
# whether to switch.
# 

!if "$(SHELLDEV)" == "1"
BUILD_PRODUCT = NT
!endif

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif
