#
# This is the makefile for the old structure on \\trango
#
!ifdef NTMAKEENV

#
# Build using BUILD.EXE (Do not edit this section of this file, edit SOURCES)
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else # NTMAKEENV

# NOTE: NMAKE Ver 1.5 or higher needed

!ifndef ROOT
# Set this to point to the root of the Nashville tree
ROOT     = ..\..\..
!endif

C1032      = 1
MAKE       = $(ROOT)\dev\tools\c1032\bin\nmake
MSDEVMK    = viruschk.mak
CFG_RETAIL = "viruschk - Win32 Release" # Name of retail configuration
CFG_DEBUG  = "viruschk - Win32 Debug"   # Name of debug configuration

!include $(ROOT)\dev\vcwrap.mk

# If you want to add dirs to the include path, do this *after*
# including vcwrap.mk:
LIB = $(LIB);$(ROOT)\dev\msdev\lib

!endif #NTMAKEENV
