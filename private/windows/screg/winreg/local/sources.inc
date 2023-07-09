!IF 0

Copyright (c) 1991  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    David J. Gilman (davegi) 15-Nov-1991

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

The notation of .\filename.c is needed so that the coff librarian doesn't
replace other objects with the same name.

!ENDIF

MAJORCOMP=screg
MINORCOMP=winreg

TARGETNAME=localreg
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=..\..\server;..\..;..\..\..\..\inc;..\..\..\..\..\inc;..\..\perflib;$(BASEDIR)\private\windows\inc

C_DEFINES=$(C_DEFINES) -DRPC_NO_WINDOWS_H -DLOCAL -D_ADVAPI32_ -DLEAK_TRACK

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=                  \
            ..\dummy.c       \
            ..\predefh.c     \
            ..\regdkey.c     \
            ..\regdval.c     \
            ..\regecls.c     \
            ..\regekey.c     \
            ..\regeval.c     \
            ..\regkey.c      \
            ..\reglukey.c    \
            ..\regqkey.c     \
            ..\regqval.c     \
            ..\regsckey.c    \
            ..\regsrkey.c    \
            ..\regclass.c    \
            ..\regvcls.c     \
            ..\regleak.c     \
            ..\regsval.c     \
            ..\stkwalk.c     \
            ..\regleak.c     \
            ..\regstate.c    \
            ..\regnccls.c


NTTARGETFILE0=                 \
            ..\predefh.c       \
            ..\regdkey.c       \
            ..\regdval.c       \
            ..\regecls.c       \
            ..\regekey.c       \
            ..\regeval.c       \
            ..\regkey.c        \
            ..\reglukey.c      \
            ..\regqkey.c       \
            ..\regqval.c       \
            ..\regsckey.c      \
            ..\regsrkey.c      \
            ..\regclass.c      \
            ..\regvcls.c       \
            ..\regleak.c       \
            ..\regsval.c       \
            ..\stkwalk.c       \
            ..\regleak.c       \
            ..\regstate.c      \
            ..\regnccls.c
