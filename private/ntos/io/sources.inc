!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=io

TARGETNAME=io
TARGETTYPE=LIBRARY

INCLUDES=..;$(BASEDIR)\private\ntos\inc;$(BASEDIR)\private\inc;$(DDK_INC_PATH)
GPSIZE=32

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -D_NTSYSTEM_

SOURCES=..\arcsec.c   \
        ..\assign.c   \
        ..\complete.c \
        ..\create.c   \
        ..\devctrl.c  \
        ..\devnode.c  \
        ..\dir.c      \
        ..\dumpctl.c  \
        ..\errorlog.c \
        ..\flunkirp.c \
        ..\fsctrl.c   \
        ..\hashirp.c  \
        ..\internal.c \
        ..\ioassert.c \
        ..\iodata.c   \
        ..\ioguid.c   \
        ..\ioinit.c   \
        ..\iosubs.c   \
        ..\ioverifier.c \
        ..\loadunld.c \
        ..\lock.c     \
        ..\mapper.c   \
        ..\misc.c     \
        ..\netboot.c  \
        ..\objsup.c   \
        ..\open.c     \
        ..\parse.c    \
        ..\pnpbusno.c \
        ..\pnpcvrt.c  \
        ..\pnpdata.c  \
        ..\pnpdd.c    \
        ..\pnpdel.c   \
        ..\pnpdma.c   \
        ..\pnpeisa.c  \
        ..\pnpenum.c  \
        ..\pnpinit.c  \
        ..\pnpioapi.c \
        ..\pnpirp.c   \
        ..\pnpirq.c   \
        ..\pnpmap.c   \
        ..\pnpmemio.c \
        ..\pnppower.c \
        ..\pnpres.c   \
        ..\pnprlist.c \
        ..\pnpsubs.c  \
        ..\qsea.c     \
        ..\qsfs.c     \
        ..\qsinfo.c   \
        ..\qsquota.c  \
        ..\query.c    \
        ..\read.c     \
        ..\report.c   \
        ..\sessnirp.c \
        ..\trackirp.c \
        ..\triage.c   \
        ..\write.c    \
        ..\dockhwp.c  \
        ..\remlock.c

NTTEST=
OPTIONAL_NTTEST=tio

PRECOMPILED_INCLUDE=..\iop.h
PRECOMPILED_PCH=iop.pch
PRECOMPILED_OBJ=iop.obj

SOURCES_USED=..\sources.inc
