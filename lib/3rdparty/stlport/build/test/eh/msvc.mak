!if !EXIST(..\..\Makefiles\nmake\config.mak)
!error No config file found, please run 'configure --help' first.
!endif

!include ..\..\Makefiles\nmake\config.mak

!ifndef COMPILER_NAME
!error No compiler set, please run 'configure --help' first and chose a compiler.
!endif

!if (("$(COMPILER_NAME)" != "vc6") && \
     ("$(COMPILER_NAME)" != "vc70") && \
     ("$(COMPILER_NAME)" != "vc71") && \
     ("$(COMPILER_NAME)" != "vc8") && \
     ("$(COMPILER_NAME)" != "vc9") && \
     ("$(COMPILER_NAME)" != "icl"))
!error '$(COMPILER_NAME)' not supported by this make file, please rerun 'configure' script and follow instructions.
!endif

SRCROOT=../..
STLPORT_DIR=../../..

!include Makefile.inc

!ifndef WITHOUT_STLPORT
INCLUDES=$(INCLUDES) /I$(STLPORT_INCLUDE_DIR) /I$(STLPORT_DIR)/src /FI warning_disable.h
!else
INCLUDES=$(INCLUDES) /I$(STLPORT_DIR)/src /FI warning_disable.h
DEFS=/DWITHOUT_STLPORT
!endif

LDSEARCH=$(LDSEARCH) /LIBPATH:$(STLPORT_LIB_DIR)

!include $(SRCROOT)/Makefiles/nmake/top.mak
