
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
MINORCOMP=kernel

TARGETTYPE=LIBRARY

C_DEFINES=$(C_DEFINES) $(USER_C_DEFINES) -DUNICODE -D_USER32_ -DNTOS_KERNEL_RUNTIME=1 -D_NTDRIVER_ -D_NTSYSTEM_DRIVER_


!IFDEF USER_PERFORMANCE
C_DEFINES=$(C_DEFINES) -DUSER_PERFORMANCE
!ENDIF

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

C_DEFINES=$(C_DEFINES) -DUSE_MIRRORING

INCLUDES=..;                                  \
         ..\..\inc;                           \
         ..\..\inc\$(ALT_PROJECT);            \
         ..\..\..\ntgdi\inc;                  \
         ..\..\..\w32inc;                     \
         $(BASEDIR)\private\ntos\inc;         \

NT_UP=0

TARGETNAME=userk
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)

NTPROFILEINPUT=yes

SOURCES_OPTIONS=-c0
NTTARGETFILE0=callcf.c strid.h msg00001.bin strid.rc

SOURCES=..\access.c   \
        ..\acons.c    \
        ..\atom.c     \
        ..\base.c     \
        ..\calcclrc.c \
        ..\caption.c  \
        ..\capture.c  \
        ..\caret.c    \
        ..\class.c    \
        ..\classchg.c \
        ..\cleanup.c  \
        ..\clipbrd.c  \
        ..\createw.c  \
        ..\cursor.c   \
        ..\dc.c       \
        ..\ddemlsvr.c \
        ..\ddetrack.c \
        ..\debug.c    \
        ..\desktop.c  \
        ..\dragdrop.c \
        ..\drawfrm.c  \
        ..\dtbitmap.c \
        ..\dwp.c      \
        ..\enumwin.c  \
        ..\event.c    \
        ..\ex.c       \
        ..\exitwin.c  \
        ..\fekbd.c    \
        ..\focusact.c \
        ..\fullscr.c  \
        ..\getset.c   \
        ..\globals.c  \
        ..\handtabl.c \
        ..\heap.c     \
        ..\help.c     \
        ..\hooks.c    \
        ..\hotkeys.c  \
        ..\hungapp.c  \
        ..\icons.c    \
        ..\imehotky.c \
        ..\inctlpan.c \
        ..\init.c     \
        ..\input.c    \
        ..\job.c      \
        ..\kbd.c      \
        kbdus.c  \
        ..\kbdlyout.c \
        ..\keyboard.c \
        ..\keyconv.c  \
        ..\libmgmt.c  \
        ..\loadbits.c \
        ..\logon.c    \
        ..\menu.c     \
        ..\menudd.c   \
        ..\metrics.c  \
        ..\minmax.c   \
        ..\miscutil.c \
        ..\mnaccel.c  \
        ..\mnapi.c    \
        ..\mnchange.c \
        ..\mncomput.c \
        ..\mncreate.c \
        ..\mndraw.c   \
        ..\mndstry.c  \
        ..\mngray.c   \
        ..\mnkey.c    \
        ..\mnloop.c   \
        ..\mnpopup.c  \
        ..\mnsel.c    \
        ..\mnstate.c  \
        ..\mnsys.c    \
        ..\movesize.c \
        ..\msgbeep.c  \
        ..\multimon.c \
        ..\ntimm.c    \
        ..\ntinput.c  \
        ..\ntstubs.c  \
        ..\paint.c    \
        ..\palette.c  \
        ..\pnp.c      \
        ..\pool.c     \
        ..\power.c    \
        ..\profile.c  \
        ..\queue.c    \
        ..\random.c   \
        ..\rare.c     \
        ..\sbapi.c    \
        ..\sbctl.c    \
        ..\scrollw.c  \
        ..\security.c \
        ..\sendmsg.c  \
        ..\server.c   \
        ..\service.c  \
        ..\showwin.c  \
        ..\sirens.c   \
        ..\snapshot.c \
        ..\spb.c      \
        ..\sprite.c   \
        ..\srvhook.c  \
        ..\srvmsg.c   \
        ..\ssend.c    \
        ..\swp.c      \
        ..\syscmd.c   \
        ..\sysmet.c   \
        ..\taskman.c  \
        ..\text.c     \
        ..\timers.c   \
        ..\tmswitch.c \
        ..\tooltips.c \
        ..\tounicod.c \
        ..\update.c   \
        ..\usergdi.c  \
        ..\validate.c \
        ..\visrgn.c   \
        ..\winable.c  \
        ..\winable2.c \
        ..\winhtky.c  \
        ..\winloop2.c \
        ..\winmgr.c   \
        ..\winprop.c  \
        ..\winsta.c   \
        ..\winwhere.c \
        ..\wmicon.c   \
        ..\xlate.c    \
        callcf.c


UMTYPE=windows
UMTEST=
UMLIBS=

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc

