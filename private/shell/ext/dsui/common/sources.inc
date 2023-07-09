!INCLUDE ..\..\dsui.inc

MAJORCOMP=dsui
MINORCOMP=common

TARGETNAME=common
TARGETTYPE=LIBRARY
TARGETPATH=obj

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_PCH=pch.pch
PRECOMPILED_OBJ=pch.obj

SOURCES=    ..\debug.cpp    \
            ..\unknown.cpp  \
            ..\atlconv.cpp  \
            ..\misc.cpp

#
# move it to dsui\lib\{winn/win95}\obj\common.lib
#

NTTARGETFILES= $(DSUI_DIR)\lib\$(PLATFORM)\$(O)\common.lib