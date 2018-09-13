##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991-96
#   All Rights Reserved.
#
##########################################################################

#
# Common include file 'sources' files ATL controls in the controls project.
#
#
# Required definitions:
#
#     CONTROLS_DIR
#        Path to controls root.
#
#     FREEBUILD
#        Set by makefile.def file.  One of the following:
#          0      - build debug
#          1      - build retail
#
#
# Definitions used if defined:
#
#     USE_LEGO
#        Define lego macros
#
#     TARGET_WIN95
#        Build component explicitly for the Win95 platform.  If this is
#        not defined, default is NT or both.
#
#     TARGET_BOTH
#        Build component to run on both platforms.
#
#     FULL_DEBUG
#        If set to 1, then debug builds will also have FULL_DEBUG defined.
#        Retail builds unaffected.
#
#     MEASURE_PERF
#        If set to 1, enables performance timing code for both debug and
#        retail builds.  This code squirts the timings for interesting
#        events to the debug terminal.
#

!if !defined(CONTROLS_DIR)
!error  CONTROLS_DIR must be defined in sources file
!endif

#
# Set private paths
#
INCLUDES        = \
		  $(SDK_INC_PATH)\atl21;\
		  $(SDK_INC_PATH)\mfc42;\
          $(INCLUDES)

#
# Set common options
#

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

C_DEFINES       = $(C_DEFINES) -DWIN32 -D_WIN32

!if !defined(TARGET_WIN95)
C_DEFINES       = $(C_DEFINES) -DNT

!  if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!  endif
!endif

!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DDEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w

# Check for missing PURE directives in interface decls (debug only)
C_DEFINES       = $(C_DEFINES) -DNO_NOVTABLE

# Define FULL_DEBUG for asserts to break by default
!  if "$(FULL_DEBUG)" == "1"
C_DEFINES       = $(C_DEFINES) -DFULL_DEBUG
!  endif
!endif

!if "$(MEASURE_PERF)" == "1"
C_DEFINES       = $(C_DEFINES) -DMEASURE_PERF
!endif

!if "$(NO_PRIVATE_HEADERS)" == "1"
C_DEFINES       = $(C_DEFINES) -DNO_PRIVATE_HEADERS
!endif

# This define helps us distinguish our development environment
C_DEFINES       = $(C_DEFINES) -DWINNT_ENV

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


