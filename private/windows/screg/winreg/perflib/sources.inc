!IF 0
**************************************************************
Sources file for building an Win32 dll.

Module Name :   Sources for regmon.dll

Author:  Russ Blake (russbl) 11/01/91

Revision:    07/30/92  (russbl)  -  remove DLLBASE

**************************************************************
!ENDIF

MAJORCOMP=winreg
MINORCOMP=perflib

TARGETNAME=perflib
TARGETPATH=obj
TARGETTYPE=LIBRARY
TARGETLIBS=

INCLUDES=..\..;..\..\prflbmsg;..\..\..\..\..\inc;..\..\..\..\inc

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\perflib.c \
        ..\perfname.c \
        ..\perfsec.c \
        ..\utils.c 

C_DEFINES=$(C_DEFINES) -DWIN32 -DSTRICT -D_ADVAPI32_
