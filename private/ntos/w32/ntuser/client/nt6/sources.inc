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

MAJORCOMP=user
MINORCOMP=client

TARGETNAME=user32
TARGETTYPE=DYNLINK
DLLENTRY=UserClientDllInitialize

TARGETPATH=obj\$(ALT_PROJECT_TARGET)
TARGETPATHLIB=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\gdi32p.lib   \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib


USE_NTDLL=1

LINKLIBS= $(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\w32umode.lib \
           ..\..\obj\$(ALT_PROJECT_TARGET)\*\userrtl.lib

COMMONTARGETS=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\user32p.lib


C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -D_USER32_ -DUNICODE -DUSE_MIRRORING

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

NTTARGETFILE1=$(COMMONTARGETS)

SOURCES_OPTIONS=-c0
NTTARGETFILE0=dispcb.c user32.def

INCLUDES=..;..\..\inc;..\..\inc\$(ALT_PROJECT);..\..\..\w32inc;$(BASEDIR)\private\ntos\inc;..\..\..\ntgdi\inc

NT_UP=0

DLLORDER=..\$(TARGETNAME).prf

SOURCES=..\res.rc     \
        ..\acons.c    \
        ..\btnctl.c   \
        ..\callback.c \
        ..\classc.c   \
        ..\cldib.c    \
        ..\clenum.c   \
        ..\clglobal.c \
        ..\clhook.c   \
        ..\client.c   \
        ..\clientw.c  \
        ..\clinit.c   \
        ..\clmenu.c   \
        ..\clmsg.c    \
        ..\clrect.c   \
        ..\clres.c    \
        ..\combo.c    \
        ..\combodir.c \
        ..\comboini.c \
        ..\connect.c  \
        ..\crecv.c    \
        ..\csendw.c   \
        ..\csrstubs.c \
        ..\drawtext.c \
        ..\ddemlcli.c \
        ..\ddemlwp.c  \
        ..\ddetrack.c \
                            ..\debug.cpp \
                            ..\dlg.cpp \
        dispcb.c      \
        ..\dlgbegin.c \
        ..\dlgend.c   \
        ..\dlgmgr.c   \
        ..\dlgmgr2.c  \
        ..\dlgmgrc.c  \
        ..\dmmnem.c   \
        ..\draw.c     \
        ..\edecrare.c \
                            ..\edge.cpp \
        ..\editec.c   \
        ..\editml.c   \
        ..\editsl.c   \
        ..\edmlrare.c \
        ..\edmlonce.c \
        ..\edslrare.c \
        ..\extract.c  \
        ..\fntsweep.c \
        ..\getsetc.c  \
        ..\handles.c  \
        ..\hdata.c    \
        ..\help.c     \
        ..\hsz.c      \
        ..\instance.c \
        ..\keybrdc.c  \
        ..\lb1.c      \
        ..\lboxctl1.c \
        ..\lboxctl2.c \
        ..\lboxctl3.c \
        ..\lboxmult.c \
        ..\lboxrare.c \
        ..\lboxvar.c  \
                            ..\list.cpp \
        ..\mdimenu.c  \
        ..\mdiwin.c   \
        ..\menuc.c    \
        ..\menuddc.c  \
        ..\mmcl.c     \
        ..\mngrayc.c  \
        ..\monitor.c  \
        ..\msgbox.c   \
        ..\ntstubs.c  \
        ..\oemxlate.c \
                                ..\parentinfo.cpp \
                                ..\persctl.cpp \
        ..\queuec.c   \
        ..\random.c   \
        ..\reader.c   \
        ..\register.c \
                                ..\resizedlg.cpp \
        ..\rtlinit.c  \
        ..\rtlres.c   \
        ..\sbapi.c    \
        ..\statctl.c  \
        ..\stdptcl.c  \
        ..\strings.c  \
        ..\util.c     \
        ..\winable.c  \
        ..\winmgrc.c  \
        ..\wow.c      \
    ..\wsprintf.c \
        ..\wstrings.c \
        ..\xact.c     \
        ..\clienta.c  \
        ..\csenda.c   \
        ..\fareast.c  \
        ..\imectl.c

UMTYPE=windows

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc
