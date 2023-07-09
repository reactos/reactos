!IF 0

Copyright (c) 1996  Microsoft Corporation

!ENDIF

MAJORCOMP=windows
MINORCOMP=mscmm

!ifndef ICMROOT
ICMROOT=$(BASEDIR)\private\ntos\w32\ntgdi\icm
!endif

!include $(ICMROOT)\icmsrc.inc

TARGETNAME=icm32
TARGETTYPE=DYNLINK
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETLIBS=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)\*\mscms.lib   \
           $(BASEDIR)\public\sdk\lib\*\kernel32.lib                      \
           ..\..\lh_open\$(ALT_PROJECT_TARGET)\obj\*\lh_open.lib         \
           ..\..\lh_core\$(ALT_PROJECT_TARGET)\obj\*\lh_core.lib

DLLBASE=@$(BASEDIR)\public\sdk\lib\coffbase.txt,icm32

NTLEGO=1

C_DEFINES=$(C_DEFINES) -DWIN32 -D__MSDOS__ -DIntelMode -DRenderInt -DMS_Icm

USE_CRTDLL=1
DLLENTRY=DllMain

INCLUDES=..\.;..\..\lh_core;..\..\lh_open

SOURCES= ..\icm32.rc    \
         ..\icm32.c     \
         ..\access.c    \
         ..\memprof.c

DLLDEF=..\$(TARGETNAME).def

