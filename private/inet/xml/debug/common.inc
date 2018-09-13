/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
!if "$(BUILD_PRODUCT)" == "NT"
FREEBUILD=1
NTDEBUG=ntsdnodbg
NTDEBUGTYPE=windbg
BUILD_ALT_DIR=
!if [if not exist obj\$(TARGET_DIRECTORY)\*.* mkdir obj\$(TARGET_DIRECTORY)] != 0
!endif
!endif


CONDITIONAL_INCLUDES= \
    winwlm.h \
    rpcerr.h \
    rpcmac.h \
    macname1.h \
    macpub.h \
    macapi.h \
    macname2.h \
    macocidl.h \
    mainwin.h \
    w16toff.hxx \
    print.h \
    regmsg16.h \
    urlmki.h \
    help.h \
    pgstup16.hxx \
    page.h \

C_DEFINES = $(C_DEFINES) \
    /D_WCTYPE_INLINE_DEFINED \
    /DVIADUCT \
    /DPRODUCT_96P \
    /DSTRICT \
    /DLINESERV \
    /DUNICODE \
    /D_UNICODE \
    /DOLEDBVER=0x0200\
    /DNASHVILLE \
    /DACTIVEX_DEBUG

!if !$(FREEBUILD)
USE_RTTI=1
MSC_OPTIMIZATION=/Od
#USER_CPP_FLAGS=/zmf
WATCOM_OPTS=/ei /zt16 /zm /zff /zgf
LINKER_NOICF=1      # ICF is s-l-o-w
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

# Win 32 Environment.
C_DEFINES = $(C_DEFINES) /D_MT

!if $(386)
NO_NTDLL=1
!else
# USE_NTDLL=1
!endif

CHECKED_ALT_DIR=1
