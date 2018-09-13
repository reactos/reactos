ALT_PROJECT=FE
ALT_PROJECT_TARGET=$(ALT_PROJECT)

MAJORCOMP=ntos
MINORCOMP=fsvga

TARGETNAME=fsvga
TARGETPATH=obj
TARGETTYPE=DRIVER

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..\..\..\..\..\inc; \
         ..\..\..\inc

NTTARGETFILE0=fsvgalog.h

SOURCES=

i386_SOURCES=..\fsvga.rc   \
             ..\fsvga.c    \
             ..\drawscrn.c \
             ..\foncache.c \
             ..\misc.c     \
             ..\port.c
