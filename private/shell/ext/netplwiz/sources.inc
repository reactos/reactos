!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources


Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1989


Revision History:

    Dave Sheldon (dsheldon)
    12-Mar-1998 copied from \private\nt\ui\mpr\mpr\sources and modified
    23-Sept-1998 merged bulk of user manager stuff

!ENDIF

SELFREGNAME     = netplwiz_selfreg.inf
USE_NT_PRODUCT_VER = 1

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED = $(SOURCES_USED) $(CCSHELL_DIR)\common.inc

MAJORCOMP=shell
MINORCOMP=accesory

TARGETNAME= netplwiz
TARGETTYPE= DYNLINK
TARGETPATH= obj
DLLENTRY=DllMain
DLLDEF=..\netplwiz.def

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\stdafx.h

# Include makefile.inc
NTTARGETFILES   = 

# Pass zero stuff
NTTARGETFILE0   =

MISCFILES=..\res\lusrmgr.msc

MSC_WARNING_LEVEL=/W3 /WX

USE_CRTDLL=0

TARGETLIBS=\
            $(BASEDIR)\public\sdk\lib\*\kernel32.lib \
            $(BASEDIR)\public\sdk\lib\*\user32.lib   \
            $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
            $(BASEDIR)\public\sdk\lib\*\gdi32.lib    \
            $(BASEDIR)\public\sdk\lib\*\shell32.lib  \
            $(BASEDIR)\public\sdk\lib\*\shell32p.lib \
            $(BASEDIR)\public\sdk\lib\*\shlwapi.lib  \
            $(BASEDIR)\public\sdk\lib\*\shlwapip.lib \
            $(BASEDIR)\public\sdk\lib\*\comctl32.lib \
            $(BASEDIR)\public\sdk\lib\*\comctlp.lib  \
            $(BASEDIR)\public\sdk\lib\*\wininet.lib  \
            $(BASEDIR)\public\sdk\lib\*\netapi32.lib \
            $(BASEDIR)\public\sdk\lib\*\objsel.lib   \
            $(BASEDIR)\public\sdk\lib\*\activeds.lib \
            $(BASEDIR)\public\sdk\lib\*\adsiid.lib   \
            $(BASEDIR)\public\sdk\lib\*\ole32.lib    \
            $(BASEDIR)\public\sdk\lib\*\oleaut32.lib \
            $(BASEDIR)\public\sdk\lib\*\secur32.lib  \
            $(BASEDIR)\public\sdk\lib\*\mpr.lib      \
            $(BASEDIR)\public\sdk\lib\*\uuid.lib     \
            $(BASEDIR)\public\sdk\lib\*\ntdsapi.lib  \
            $(BASEDIR)\public\sdk\lib\*\rasdlg.lib   \
            $(BASEDIR)\public\sdk\lib\*\rasapi32.lib \
            $(BASEDIR)\private\shell\lib\$O\stocklib.lib \
            $(BASEDIR)\private\shell\lib\$O\stock5.lib \
            $(BASEDIR)\public\sdk\lib\*\msdasc.lib

C_DEFINES=-DWINNT -DUNICODE -D_UNICODE -DWINVER=0x0500 -D_WIN32_WINNT=0x0500 -DUSE_MIRRORING
#-DDONT_JOIN

DELAYLOAD       = WININET.DLL

INCLUDES=   ..\; \
            $(BASEDIR)\private\windows\inc; \
            $(BASEDIR)\private\shell\inc; \
            $(BASEDIR)\public\sdk\inc\atl21; \
            $(BASEDIR)\public\sdk\inc; \
            $(BASEDIR)\public\sdk\inc\crt; \
            $(BASEDIR)\private\INC; \
            $(BASEDIR)\private\net\INC; \

SOURCES=\
        ..\resource.rc \
        ..\dll.cpp \
        ..\dialog.cpp\
        ..\pathpg.cpp \
        ..\fldrpg.cpp \
        ..\netfinpg.cpp \
        ..\ftppg.cpp \
        ..\password.cpp \
        ..\misc.cpp \
        ..\netwiz.cpp \
        ..\domain.cpp \
        ..\join.cpp \
        ..\debug.cpp \
        ..\ulistpg.cpp \
        ..\data.cpp \
        ..\unpage.cpp \
        ..\pwpage.cpp \
        ..\grppage.cpp \
        ..\netpage.cpp \
        ..\misc.cpp \
        ..\autolog.cpp \
        ..\secdlg.cpp \
        ..\advpg.cpp \
        ..\userinfo.cpp \
        ..\grpinfo.cpp \
        ..\usercom.cpp \
        ..\mnddlg.cpp \
        ..\netpldat.cpp

