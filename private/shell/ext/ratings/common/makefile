# Makefile : Build the common network provider libraries.
##########################################################################
#
#   Microsoft Confidential
#   Copyright (C) Microsoft Corporation 1991
#   All Rights Reserved.
#
##########################################################################
!if "$(NTMAKEENV)" != ""
#
# This section is for the NT build ONLY.
# For NT build, change the 'sources' or 'dirs' file, not this makefile
#
!INCLUDE $(NTMAKEENV)\makefile.def

!else
#
# Non-NT build makefile start HERE.
#

NAME=npcommon
DEPENDNAME=depend32.mk
ROOT=..\..\..\..

WANT_C1032=TRUE
IS_SDK=TRUE

!ifdef VERDIR
ROOT=..\$(ROOT)
!endif

PCHOBJ0=bufbase.obj buffer.obj npassert.obj alloc.obj npcrit.obj
PCHOBJ1=strcmp.obj \
		strupr.obj \
		stratol.obj \
		strassgn.obj \
		strncpy.obj \
		strrchr.obj \
		stricmp.obj \
		strspn.obj \
		stris.obj \
		string.obj \
		strnchar.obj \
		strload.obj \
		strprof.obj \
		strrss.obj \
		strchr.obj \
		strcat.obj \
		strmisc.obj \
		strnicmp.obj \
		strcspn.obj \
		strdss.obj \
		strtok.obj \
		strinsrt.obj \
		strstr.obj \
		stristr.obj \
		stratoi.obj \
		strqss.obj \
		strncmp.obj \
		istr.obj \
		istraux.obj \
		strparty.obj \
		npmsg.obj \
		npgenerr.obj

PCHOBJ2=iconlbox.obj regentry.obj convtime.obj dostime.obj timedata.obj
PCHOBJ3=purecall.obj
PCHOBJ4=sched.obj
PCHOBJ5=str.obj chr.obj cmp.obj spn.obj upr.obj cpycat.obj

BUILDDLL=TRUE
BUILDLIB=TRUE
APPEXT=lib

INCFLAGS=-P$$(COMMON)=$(COMMON)
CUSTOMCFLAGS=-GF -Oxs -Gy

!include $(ROOT)\inet\ohare\ratings\common32.mk

$(NAME).pch:    $(RATINGSROOT)\inc\npdefs.h

INCLUDE=$(COMMON)\h;$(INCLUDE)

!endif    # !if NTMAKEENV
