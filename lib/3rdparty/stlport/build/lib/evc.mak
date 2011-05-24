!if !EXIST(..\Makefiles\nmake\config.mak)
!error No config file found, please run 'configure --help' first.
!endif

!include ..\Makefiles\nmake\config.mak

!ifndef COMPILER_NAME
!error No compiler set, please run 'configure --help' first and chose a compiler.
!endif

!if ("$(COMPILER_NAME)" != "evc3" && \
     "$(COMPILER_NAME)" != "evc4" && \
     "$(COMPILER_NAME)" != "evc8" && \
     "$(COMPILER_NAME)" != "evc9")
!error You picked the wrong makefile, please rerun configure script and follow the instructions.
!endif

SRCROOT=..

STLPORT_INCLUDE_DIR = ../../stlport
!include Makefile.inc

CROSS_COMPILING=1

DEFS_REL = /D_STLP_USE_DYNAMIC_LIB
DEFS_DBG = /D_STLP_USE_DYNAMIC_LIB
DEFS_STLDBG = /D_STLP_USE_DYNAMIC_LIB
DEFS_STATIC_REL = /D_STLP_USE_STATIC_LIB
DEFS_STATIC_DBG = /D_STLP_USE_STATIC_LIB
DEFS_STATIC_STLDBG = /D_STLP_USE_STATIC_LIB

INCLUDES=$(INCLUDES) /I "$(STLPORT_INCLUDE_DIR)"

LDSEARCH=$(LDSEARCH) /LIBPATH:$(STLPORT_LIB_DIR)
RC_FLAGS_REL = /I "$(STLPORT_INCLUDE_DIR)" /D "COMP=$(COMPILER_NAME)"
RC_FLAGS_DBG = /I "$(STLPORT_INCLUDE_DIR)" /D "COMP=$(COMPILER_NAME)"
RC_FLAGS_STLDBG = /I "$(STLPORT_INCLUDE_DIR)" /D "COMP=$(COMPILER_NAME)"

!include $(SRCROOT)/Makefiles/nmake/top.mak
