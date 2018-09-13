!IF 0

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sources.inc.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David J. Gilman (davegi) 15-Nov-1991

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=winlogon
MINORCOMP=winreg

TARGETNAME=winreg
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..;..\..;..\..\..\..\inc;..\..\..\..\..\inc;..\..\perflib;$(BASEDIR)\private\windows\inc

C_DEFINES= -DRPC_NO_WINDOWS_H -DREMOTE_NOTIFICATION_DISABLED -DLEAK_TRACK

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=                \
            ..\init.c      \
            ..\predefh.c   \
            ..\regdkey.c   \
            ..\regdval.c   \
            ..\regecls.c   \
            ..\regekey.c   \
            ..\regeval.c   \
            ..\regkey.c    \
            ..\reglukey.c  \
            ..\regnckey.c  \
            ..\regqkey.c   \
            ..\regqval.c   \
            ..\regrpc_s.c  \
            ..\regsckey.c  \
            ..\regsrkey.c  \
            ..\regsval.c   \
            ..\regsec.c    \
            ..\regclass.c  \
            ..\support.c   \
            ..\shutdown.c  \
            ..\shutinit.c  \
            ..\stkwalk.c   \
            ..\regleak.c

