!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

!ENDIF

SEC_DIR = ..\..
SECLIB_PATH = $(SEC_DIR)\lib

C_DEFINES   = $(C_DEFINES) -DSTRICT -DUNICODE -D_UNICODE

CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) \
    macocidl.h  \
    winwlm.h    \
    rpcerr.h    \
    rpcmac.h    \
    macname1.h  \
    macpub.h    \
    macapi.h    \
    macname2.h  \

USE_ATL=1
ATL_VER=21

INCLUDES=\
    ..;                             \
    $(SEC_DIR)\inc;                 \
    $(BASEDIR)\private\windows\inc; \

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

#
# Define PROFILE to enable IceCAP profiling.
#
!IF "$(PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) -DPROFILE
USER_C_FLAGS=$(USER_C_FLAGS) /Gh /Zi
TARGETLIBS=$(TARGETLIBS) $(SDK_LIB_PATH)\icap.lib
!ENDIF
