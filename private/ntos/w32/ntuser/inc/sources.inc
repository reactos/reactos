#****************************** File Header ******************************\
# File Name: Sources.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# This file is used to build generated header files (specifed in NTTARGETFILE0).
#  Building rules are specified in MAKEFILE.INC.
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/
PASS0ONLY=1
NOPASS0=1

TARGETTYPE=NOTARGET
SOURCES=
TARGETPATH=

SOURCES_OPTIONS=-c0
NTTARGETFILE0=cscall.h scf.lst callback.h cb.lst msgdef.h msgdef.lst

SYNCHRONIZE_BLOCK=1

C_DEFINES=$(C_DEFINES) -DUSE_MIRRORING

SOURCES_USED=..\sources.inc

