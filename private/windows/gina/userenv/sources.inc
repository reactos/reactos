
TARGETNAME=userenv
TARGETPATH=$(BASEDIR)\public\sdk\lib\$(ALT_PROJECT_TARGET)
TARGETTYPE=DYNLINK

DLLDEF=..\userenv.def
DLLENTRY=LibMain

UMTYPE=windows

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

PRECOMPILED_INCLUDE=..\uenv.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

INCLUDES= \
    ..;\
    $(BASEDIR)\private\windows\inc; \
    $(BASEDIR)\private\inc;         \
    $(BASEDIR)\private\shell\inc;

C_DEFINES=-DWIN32 -D_WIN32 -DUNICODE -D_USERENV_

USE_CRTDLL=1

SOURCES= ..\uevents.mc  \
         ..\copydir.c   \
         ..\debug.c     \
         ..\dllload.c   \
         ..\envvar.c    \
         ..\events.c    \
         ..\globals.c   \
         ..\gpnotif.c   \
         ..\gpt.c       \
         ..\hkcr.c      \
         ..\migrate.c   \
         ..\policy.c    \
         ..\profile.c   \
         ..\setup.c     \
         ..\sid.c       \
         ..\stubs.c     \
         ..\userenv.c   \
         ..\userenv.rc  \
         ..\userdiff.c  \
         ..\util.c


TARGETLIBS=                                             \
       $(BASEDIR)\public\sdk\lib\*\kernl32p.lib         \
       $(BASEDIR)\public\sdk\lib\*\advapi32.lib         \
       $(BASEDIR)\public\sdk\lib\*\user32.lib           \
       $(BASEDIR)\public\sdk\lib\*\uuid.lib
