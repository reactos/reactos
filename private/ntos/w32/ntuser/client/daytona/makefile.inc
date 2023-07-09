#****************************** File Header ******************************\
# File Name: makefile.inc
#
# Copyright (c) 1985-96, Microsoft Corporation
#
# Project specific MAKEFILE.INC
#
# History:
# Feb-14-1996 GerardoB Created
#***************************************************************************/
!include ..\makefile.inc

!ifdef JANUS
..\strid.rc ..\strid.h ..\msg00001.bin: ..\strid.mc
    mc -v -U -r ..\ -h ..\ ..\strid.mc
!endif
