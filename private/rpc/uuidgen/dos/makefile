# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
#                     Microsoft RPC
#            Copyright(c) Microsoft Corp., 1994
#
# --------------------------------------------------------------------
# --------------------------------------------------------------------
#
# File : makefile.
#
# Title : Makefile for the DOS UUIDGEN program.
#
# History :
# --------------------------------------------------------------------

!ifndef RPC
!error	- You forgot to set your build environment
!endif

DOS=1

!include ..\rules.mk

OBJS = \
   uuidgen.obj \
   uuidfmt.obj


# --------------------------------------------------------------------
# These are the targets required by the build process.

all : uuidgen.exe

tree :
    copy uuidgen.exe $(RPCDIST)\dos\bin

depend :
    $(INCLUDES) $(INCLUDESFLAGS) ..\*.c > depend.mk

# --------------------------------------------------------------------
# Targets

uuidgen.exe: uuidgen.obj $(RPC)\runtime\bin\dos\rpc.lib uuidfmt.obj
    $(LINK) $(LINKFLAGS) /stack:4000 /pm:vio uuidgen+uuidfmt,,,\
            $(RPC)\runtime\bin\dos\rpc+$(RPC)\ndrmem\dos\rpcndr+$(DOS_LIB)\llibce.lib;

# --------------------------------------------------------------------

!include "depend.mk"

