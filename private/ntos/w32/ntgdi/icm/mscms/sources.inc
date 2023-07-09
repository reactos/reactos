!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

!ifndef ICMROOT
ICMROOT=$(BASEDIR)\private\ntos\w32\ntgdi\icm
!endif

!include $(ICMROOT)\icmsrc.inc

TARGETNAME=mscms
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETTYPE=DYNLINK
DLLBASE=@$(BASEDIR)\public\sdk\lib\coffbase.txt,mscms

INCLUDES=..\.;$(ICMROOT)\inc;$(INCLUDES)

DLLENTRY=DllEntryPoint
NTLEGO=1
NTKEEPRESOURCETMPFILES=1
USE_CRTDLL=1

C_DEFINES=$(C_DEFINES) -DLITTLE_ENDIAN

SOURCES=\
    ..\dllentry.c          \
    ..\globals.c           \
    ..\profile.c           \
    ..\object.c            \
    ..\color.c             \
    ..\profman.c           \
    ..\ps2wrap.c           \
    ..\mscms.rc

TARGETLIBS=$(BASEDIR)\public\sdk\lib\*\kernel32.lib \
           $(BASEDIR)\public\sdk\lib\*\advapi32.lib \
           $(BASEDIR)\public\sdk\lib\*\winspool.lib \
           $(BASEDIR)\public\sdk\lib\*\user32.lib

PRECOMPILED_INCLUDE=..\mscms.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

DLLDEF=..\$(TARGETNAME).def

