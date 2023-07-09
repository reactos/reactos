#############################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991
#   All Rights Reserved.
#
#   RATINGS project makefile include
#
#############################################################################
MASM6 = TRUE

IS_SDK=TRUE
IS_DDK=TRUE

COMMON=$(ROOT)\net\user\common

#   Include master makefile

!ifndef PROPROOT
PROPROOT = $(ROOT)\dev\bin
!endif

!ifndef RATINGSROOT
RATINGSROOT = $(ROOT)\inet\ohare\ratings
!endif

!include $(ROOT)\dev\master.mk

#   Add the local include/lib directories

INCLUDE = $(RATINGSROOT)\inc;$(DEVROOT)\inc;$(INCLUDE)
LIB=$(LIB);$(DEVROOT)\lib
