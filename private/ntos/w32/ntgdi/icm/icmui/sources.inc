!IF 0

Build Control File for the ICM 2.0 User Interface DLL

Copyright (c) 1996, 1997 by Microsoft Corporation.  All Rights Reserved

A Pretty Penny Enterprises Production

Change History:
Unknown creation date, et al.
01-08-97    KjelgaardR@acm.org  Had to add Gdi32 library...

        ..\ProfProp.CPP \
        ..\PropDlg.CPP  \

!ENDIF

!ifndef ICMROOT
ICMROOT=$(BASEDIR)\private\ntos\w32\ntgdi\icm
!endif

!include $(ICMROOT)\icmsrc.inc

UMTYPE=windows
TARGETNAME=icmui
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETTYPE=DYNLINK
DLLBASE=@$(BASEDIR)\public\sdk\lib\coffbase.txt,icmui

INCLUDES=..\.;$(ICMROOT)\inc;$(ICMROOT)\mscms;$(INCLUDES)

DLLENTRY=_DllMainCRTStartup
NTLEGO=1
NTKEEPRESOURCETMPFILES=1

USE_CRTDLL=1


C_DEFINES=$(C_DEFINES) -DWINVER=0x0500

SOURCES=..\Dialog.CPP   \
        ..\Profile.CPP  \
        ..\ProfInfo.CPP \
        ..\ProfAssoc.CPP\
        ..\PropPage.CPP \
        ..\ShellExt.CPP \
        ..\StringAr.CPP \
        ..\DevProp.CPP  \
        ..\AppUi.CPP    \
        ..\IcmUi.rc

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\shell32.lib  \
           $(BASEDIR)\public\sdk\lib\*\winspool.lib \
           $(BASEDIR)\public\sdk\lib\*\mscms.lib    \
           $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
           $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
           $(BASEDIR)\public\sdk\lib\*\user32p.lib  \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
           $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
           $(BASEDIR)\public\sdk\lib\*\comdlg32.lib

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\IcmUi.H
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=..\$(TARGETNAME).def
