CSCUI_DIR = $(BASEDIR)\private\shell\ext\cscui

C_DEFINES = $(C_DEFINES) -DSTRICT

CONDITIONAL_INCLUDES=$(CONDITIONAL_INCLUDES) \
    macocidl.h  \
    winwlm.h    \
    rpcerr.h    \
    rpcmac.h    \
    macname1.h  \
    macpub.h    \
    macapi.h    \
    macname2.h  \
    new         \
    atlbase.h   \
    atlconv.cpp \
    ifs.h       \
    defs.h      \
    vmm.h       \

USE_ATL=1
ATL_VER=21

INCLUDES=\
    ..\.;                               \
    $(CSCUI_DIR)\inc;                   \
    $(CSCUI_DIR)\lib;                   \
    $(CSCUI_DIR)\dll;                   \
    $(CSCUI_DIR)\viewer;                \
    $(BASEDIR)\private\shell\inc;       \

!IF "$(TARGET_WIN95)" == "1"
CHICAGO_PRODUCT=1
CSCUILIB_PATH = $(CSCUI_DIR)\lib\win95
!ELSE
CSCUILIB_PATH = $(CSCUI_DIR)\lib\winnt
C_DEFINES     = $(C_DEFINES) -DUNICODE -D_UNICODE
!ENDIF

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

!IF !$(FREEBUILD)
C_DEFINES=$(C_DEFINES) -DDEBUG
!ENDIF

!IF "$(ICEPICK)" == "1"
USE_PDB=1
LINKER_FLAGS=$(LINKER_FLAGS) /DEBUGTYPE:CV,FIXUP /DBGIMPLIB /DEBUG /OPT:REF
USER_C_FLAGS=$(USER_C_FLAGS) /Zi
!ENDIF

!IF "$(PROFILE)" == "1"
C_DEFINES=$(C_DEFINES) -DPROFILE
USER_C_FLAGS=$(USER_C_FLAGS) /Gh /Zi
!ENDIF
