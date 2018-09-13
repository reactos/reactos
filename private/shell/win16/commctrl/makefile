# Makefile : Build the common controls
##########################################################################
#
#       Microsoft Confidential
#       Copyright (C) Microsoft Corporation 1991
#       All Rights Reserved.
#
##########################################################################

RES_DIR         = ..
PRIVINC         = ctlspriv
ROOT            = ..\..\..
CCSHELL_DIR     = $(ROOT)\shell
DLLBASE         = PREFBASE
THUNKDIR        = $(CCSHELL_DIR)\thunk

!ifdef VERDIR
ROOT            = ..\$(ROOT)

!if "$(SUBVERDIR)" == "debug31" || "$(SUBVERDIR)" == "retail31"
WIN31           = TRUE
!endif

!if "$(SUBVERDIR)" == "debugie" || "$(SUBVERDIR)" == "retailie"
WIN31           = TRUE
IEWIN31         = TRUE
IEWIN31_25      = TRUE
!endif

CLEANLIST       = *.pdb

!else

VERSIONLIST     = debug retail
!if "$(BUILD)" == "clean"
VERSIONLIST     = $(VERSIONLIST) debug31 retail31
!endif

!endif  # VERDIR


#
# Note that WIN31 build is called comctl31, and the IEWIN31 build is comctlie
#
NAME            = commctrl
!ifdef WIN31
!ifdef IEWIN31
!ifdef IEWIN31_25
NAME            = comctlie
CFLAGS          = -DWIN31 -DIEWIN31 -DIEWIN31_25 $(CFLAGS)
AFLAGS          = -DWIN31 -DIEWIN31 -DIEWIN31_25 $(AFLAGS)
RCFLAGS         = -31 -DWIN31 -DIEWIN31 -DIEWIN31_25 $(RCFLAGS)
DEFNAME         = comctl25.def
!else
NAME            = comctlie
CFLAGS          = -DWIN31 -DIEWIN31 $(CFLAGS)
AFLAGS          = -DWIN31 -DIEWIN31 $(AFLAGS)
RCFLAGS         = -31 -DWIN31 -DIEWIN31 $(RCFLAGS)
!endif
!else
NAME            = comctl31
CFLAGS          = -DWIN31 $(CFLAGS)
AFLAGS          = -DWIN31 $(AFLAGS)
RCFLAGS         = -31 -DWIN31 $(RCFLAGS)
!endif
RESNAME         = commctrl.res
RCNAME          = commctrl.rc
RCVNAME         = commctrl.rcv
!else   # WIN31
!ifdef  DBCS
CFLAGS          = $(CFLAGS) -DFE_IME -DIMC32
!endif  # DBCS
!endif  # WIN31

!ifdef  WINDOWS_ME
CFLAGS          = $(CFLAGS) -DWINDOWS_ME -D$(WINDOWS_ME)
!endif

AFLAGS          = $(AFLAGS)

!ifdef  LIST
MAKELIST        = TRUE
!endif

!ifdef WIN31

!ifdef IEWIN31_25

#========================================================================
# 16-bit Windows 3.1 IExplore
#
#  This grouping of segments is the result of serious segment tuning.
# Please don't move thing around unless you are absolutely sure that
# you are doing the right thing.
#
# _INIT       -- Loaded at boot time supposed be discarded (not yet done).
# _TEXT       -- Resident portion (keep this minimum)
# DOSBOX_TEXT -- Code for MS-DOS box (except property sheet code)
# PRSHT_TEXT  -- 16-bit property sheet code (for MS-DOS box)
# TV_TEXT     -- Treeview code
# LV_TEXT     -- Listview code
# RARE1_TEXT  -- Other code (rarely used)
# RARE2_TEXT  -- Other code (rarely used)
#
#========================================================================
MISCOBJ0        = dllentry.obj
CODESEG0        = _TEXT
PCHOBJ0         = mem.obj da.obj cstrings.obj commctrl.obj cutils.obj
CODESEG1        = PRSHT_TEXT
PCHOBJ1         = prsht.obj prsht1.obj tab.obj dlgmgr31.obj wmdraw.obj \
                  draw.obj image.obj
CODESEG2        = TV_TEXT
PCHOBJ2         = treeview.obj tvmem.obj tvpaint.obj tvscroll.obj
CODESEG3        = LV_TEXT
PCHOBJ3         = listview.obj lvrept.obj lvicon.obj lvlist.obj lvsmall.obj
CODESEG4        = RARE2_TEXT
PCHOBJ4         = updown.obj strings.obj header2.obj
CODESEG5        = PROGRESS_TEXT
PCHOBJ5         = progress.obj trackbar.obj
PCHOBJ6         = toolbar2.obj tooltips.obj rebar.obj notify.obj trackme.obj ctl3d.obj 
CODESEG6        = TOOLBAR_TEXT

LIB0            = LIBW31 MNOCRTDW MDLLCEW

!else #IEWIN31_25

#========================================================================
# 16-bit Windows 3.1 Compatible
#
#  This grouping of segments is the result of serious segment tuning.
# Please don't move thing around unless you are absolutely sure that
# you are doing the right thing.
#
# _INIT       -- Loaded at boot time supposed be discarded (not yet done).
# _TEXT       -- Resident portion (keep this minimum)
# DOSBOX_TEXT -- Code for MS-DOS box (except property sheet code)
# PRSHT_TEXT  -- 16-bit property sheet code (for MS-DOS box)
# TV_TEXT     -- Treeview code
# LV_TEXT     -- Listview code
# RARE1_TEXT  -- Other code (rarely used)
# RARE2_TEXT  -- Other code (rarely used)
#
#========================================================================
MISCOBJ0        = dllentry.obj
CODESEG0        = _TEXT
PCHOBJ0         = mem.obj da.obj cstrings.obj commctrl.obj cutils.obj
CODESEG1        = PRSHT_TEXT
PCHOBJ1         = prsht.obj prsht1.obj tab.obj dlgmgr31.obj wmdraw.obj \
                  draw.obj listview.obj updown.obj progress.obj

LIB0            = LIBW31 MNOCRTDW MDLLCEW

!endif #IEWIN31_25
!else # WIN31

#========================================================================
# 16-bit
#
#  This grouping of segments is the result of serious segment tuning.
# Please don't move thing around unless you are absolutely sure that
# you are doing the right thing.
#
# _INIT       -- Loaded at boot time supposed be discarded (not yet done).
# _TEXT       -- Resident portion (keep this minimum)
# DOSBOX_TEXT -- Code for MS-DOS box (except property sheet code)
# PRSHT_TEXT  -- 16-bit property sheet code (for MS-DOS box)
# TV_TEXT     -- Treeview code
# LV_TEXT     -- Listview code
# RARE1_TEXT  -- Other code (rarely used)
# RARE2_TEXT  -- Other code (rarely used)
#
#========================================================================
MISCOBJ0        = dllentry.obj
THKOBJ0         = Cctl1632.obj
CODESEG0        = _TEXT
PCHOBJ0         = mem.obj da.obj cstrings.obj commctrl.obj
CODESEG1        = DOSBOX_TEXT
PCHOBJ1         = toolbar.obj cutils.obj tooltips.obj
CODESEG2        = PRSHT_TEXT
PCHOBJ2         = prsht.obj prsht1.obj tab.obj image.obj hotkey.obj
CODESEG3        = TV_TEXT
PCHOBJ3         = treeview.obj tvmem.obj tvpaint.obj tvscroll.obj
CODESEG4        = LV_TEXT
PCHOBJ4         = listview.obj lvrept.obj lvicon.obj lvlist.obj lvsmall.obj
CODESEG5        = RARE1_TEXT
PCHOBJ5         = btnlist.obj draglist.obj menuhelp.obj progress.obj status.obj
CODESEG6        = RARE2_TEXT
PCHOBJ6         = tbcust.obj trackbar.obj updown.obj strings.obj header2.obj

LIB0            = LIBW MNOCRTDW MDLLCEW
!ifdef DBCS
LIB0            = $(LIB0) IMM
!endif
!endif # WIN31

#========================================================================

BUILDDLL        = TRUE

!include $(CCSHELL_DIR)\common.mk

INCLUDE         = $(ROOT)\shell\win16\inc;$(SRCDIR);$(INCLUDE)


!ifdef VERDIR

$(NAME).res:    $(SRCDIR)\mvbtn.cur \
                $(SRCDIR)\copy.cur $(SRCDIR)\move.cur $(SRCDIR)\insert.ico

#
# Rule to generate source file dependencies
#

$(SRCDIR)\depend.mk::
!if "$(BUILD)" == "depend"
        echo # > $@
        echo # Warning: This file is generated automatically. >> $@
        echo # >> $@
        echo. >> $@
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< | sed "s/pch.obj/pch.obj .\\ctlspriv.pch/" >> $@
-D..
pch.c
<<
        $(INCLUDES) $(INCLUDES_SWITCHES) @<< >> $@
-D..
-nctlspriv.h ..\*.c ..\*.asm
<<
!else
        @echo Use "nmake BUILD=depend" to build dependencies.
!endif


!else

!if "$(BUILD)" != "clean"
all:    debug31 retail31

debug31:
        mkdir $@
        cd $@
        $(MAKE) /l BUILD="$(BUILD)" VERDIR=debug SUBVERDIR=$@ /f..\$(COMMONMKFILE)
        cd ..

retail31:
        mkdir $@
        cd $@
        $(MAKE) /l BUILD="$(BUILD)" VERDIR=retail SUBVERDIR=$@ /f..\$(COMMONMKFILE)
        cd ..

debugie:
        mkdir $@
        cd $@
        $(MAKE) /l BUILD="$(BUILD)" VERDIR=debug SUBVERDIR=$@ /f..\$(COMMONMKFILE)
        cd ..

retailie:
        mkdir $@
        cd $@
        $(MAKE) /l BUILD="$(BUILD)" VERDIR=retail SUBVERDIR=$@ /f..\$(COMMONMKFILE)
        cd ..

!endif

!endif # !VERDIR
