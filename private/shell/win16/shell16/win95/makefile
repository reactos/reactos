##########################################################################
#
#	Microsoft Confidential
#	Copyright (C) Microsoft Corporation 1991
#	All Rights Reserved.
#
##########################################################################

NAME=shell
RCVNAME = shell.rcv
RCNAME  = shell.rc
DEFNAME = shell.def
PRIVINC=shprv
ROOT=..\..\..\..
RES_DIR=..\..
SRCDIR=..\..
BUILDDLL=TRUE

!ifdef VERDIR
ROOT=..\$(ROOT)
CLEANLIST=*.pdb
!endif

#========================================================================
# 16-bit
#
#  This grouping of segments is the result of some segment tuning.
# Please don't move thing around unless you are absolutely sure that
# you are doing the right thing.
#
# _TEXT         -- Resident portion (keep this minimum)
# THUNK_TEXT    -- 16-bit/32-bit thunking related code
# WIN31_TEXT    -- Win31 API entries
# RARE_TEXT     -- Rarely used code (if any)
#
#========================================================================
!ifndef OPK2
CFLAGS= $(CFLAGS) /DNASH
!endif

OPK2=1
CFLAGS= $(CFLAGS) /DT_DROCH
# /Fc



## shstart.obj MUST BE FIRST. <chee>
FIRSTOBJS=shstart.obj
THKOBJ0=shl1632.obj shl3216.obj
CODESEG0=_TEXT
PCHOBJ0=cbthook.obj init.obj
CODESEG1=THUNK_TEXT
PCHOBJ1=cpl16.obj prt16.obj pifmgr.obj
CODESEG2=WIN31_TEXT
PCHOBJ2= dragdrop.obj dbf.obj expenv.obj util.obj #path.obj
CODESEG3=WIN31_TEXT
PCHOBJ3= extract.obj
CODESEG4=WIN31_TEXT
PCHOBJ4=start.obj msgbox.obj ## os.obj futil.obj
CODESEG5=FORMAT_TEXT
PCHOBJ5=format.obj sedlg.obj
CODESEG6=FORMAT_TEXT2
PCHOBJ6=chkdsk.obj

!ifdef DBCS
PCHOBJ4=$(PCHOBJ4) shlexec.obj
!endif

LIB0=libw mdllcew commctrl

# Always enable frosting...
CFLAGS = $(CFLAGS) -DFROSTING -DOPK2
RCFLAGS = $(RCFLAGS) -DFROSTING -DOPK2
!if "$(DBCS)"=="NEC"
CFLAGS = $(CFLAGS) -DNEC_98
!endif

!include $(ROOT)\shell\common.mk
INCLUDE=$(ROOT)\shell\win16\inc;$(INCLUDE);$(ROOT)\dev\sdk\inc

shstart.obj: ..\..\shstart.asm

shell.res : ..\..\format.rc ..\..\format.h  ..\..\shell.rc ..\..\shell.rcv \
!ifdef OPK2
	    ..\..\chkdlg2.ico ..\..\chkdlg3.ico \
!endif
	    ..\..\check.ico ..\..\checkdlg.ico

chkdsk.obj  : ..\..\chkdsk.c ..\..\format.h $(ROOT)\dev\inc16\dskmaint.h

format.obj : ..\..\format.c ..\..\format.h $(ROOT)\dev\inc16\dskmaint.h

sedlg.obj : ..\..\sedlg.c ..\..\format.h $(ROOT)\dev\inc16\dskmaint.h




		
