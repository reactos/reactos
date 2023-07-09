!IF 0

Copyright (c) 1994  Microsoft Corporation

Module Name:

    wininet.inc

Abstract:

    Common makefile contents for wininet project

Author:

    Richard L Firth (rfirth) 10-Feb-1996

Revision History:

    10-Feb-1996 rfirth
        Created

!ENDIF

#
# where to place binaries
#

!INCLUDE place.inc

#
# paths
#

!IFNDEF BASEDIR
BASEDIR=$(_NTDRIVE)$(_NTROOT)
!ENDIF

!IFNDEF SDKINC
SDKINC=$(BASEDIR)\public\sdk\inc
!ENDIF

#
# where intermediate libs go
#

!IFDEF LIBDIR
!ERROR LIBDIR already defined - choose new name
!ENDIF

LIBDIR=..\lib\$(_OBJ_DIR)

#
# if TARGETPATH is not defined (in the paths that build libraries) then this
# is the default target path
#

!IFNDEF TARGETPATH
TARGETPATH=$(LIBDIR)
TARGETTYPE=LIBRARY
!ENDIF

#
# what it is that we're building
#

MAJORCOMP=wininet

USE_NOLIBS=1
NO_NTDLL=1
# Use the stripped down version for the crt
USE_IERT=1

#
# compiler definitions
#

C_DEFINES=$(C_DEFINES) -D_WINX32_ -DTHREAD_POOL
#C_DEFINES=$(C_DEFINES) -D_WINX32_

# If we are building for X86 we use w95fiber.dll or NT kernel fibers.
!IF "$(PROCESSOR_ARCHITECTURE)"=="x86"
C_DEFINES=$(C_DEFINES) -DW95_FIBERS
!ENDIF

# Get same version from NT or IE builds.
C_DEFINES=$(C_DEFINES) -DNASHVILLE

!IFDEF USE_ICECAP
C_DEFINES=$(C_DEFINES) -DICECAP
!ENDIF

#
# Set debugging level
#

!IF "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
C_DEFINES=$(C_DEFINES) -DENABLE_DEBUG -DINET_DEBUG=1

#
# remove maximal diagnostics for customer debug version based on environment
# variable
#

!IF "$(FULL_DEBUG)" != ""
C_DEFINES=$(C_DEFINES) -DUSE_DEBUG_MEMORY
!ELSE
C_DEFINES=$(C_DEFINES) -DRETAIL_LOGGING
!IF "$(BUILD_PRODUCT)" != "IE"
C_DEFINES=$(C_DEFINES) -DDISABLE_ASSERTS
!ENDIF
!ENDIF

!ELSE IF "$(ALMOSTRETAIL)" != ""
C_DEFINES=$(C_DEFINES) -DENABLE_DEBUG
!ENDIF


#
# performance diagnostics
#

!IF "$(PERF_BUILD)" != ""
C_DEFINES=$(C_DEFINES) -DUSE_PERF_DIAG
!ENDIF

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

#
# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
#

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400

#
# compiler warning flags
#

MSC_WARNING_LEVEL=/W3 /WX

#
# precompiled header options
#

!IFNDEF WININET_PCH

PRECOMPILED_OPTION=/Fp..\inc\$(_OBJ_DIR)\*\wininetp.pch /Yuwininetp.h
PRECOMPILED_CXX=1

!ENDIF

CONDITIONAL_INCLUDES = \
        winwlm.h \
        macwin32.h \
        ia64inst.h \
        pshpck16.h \
        rpcerr.h \
        rpcmac.h \
        macname1.h \
        macpub.h \
        macapi.h \
        macname2.h
