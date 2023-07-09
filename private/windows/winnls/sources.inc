!IF 0

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    source files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

Author:

    Julie Bennett (julieb) 31-May-1991

!ENDIF


MAJORCOMP = nlsapi
MINORCOMP =

TARGETNAME = nlslib
TARGETPATH = ..\..\obj\$(ALT_PROJECT_TARGET)
TARGETTYPE = LIBRARY

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL = /W3
!ENDIF
MSC_WARNING_LEVEL = $(MSC_WARNING_LEVEL) /WX

C_DEFINES = $(C_DEFINES) -DUNICODE -D_KERNEL32_

INCLUDES = ..\..\inc;..\..\base\inc;$(BASEDIR)\private\inc

SOURCES = ..\ansi.c       \
          ..\csrlocal.c   \
          ..\csrtables.c  \
          ..\datetime.c   \
          ..\enum.c       \
          ..\init.c       \
          ..\locale.c     \
          ..\map.c        \
          ..\mbcs.c       \
          ..\number.c     \
          ..\section.c    \
          ..\string.c     \
          ..\tables.c     \
          ..\utf.c        \
          ..\util.c       \
          ..\security.c
