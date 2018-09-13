/**************************************************************************
NAME

    globals.c -- global variables for recorder

SYNOPSIS



DESCRIPTION


RETURNS


CAUTIONS

    ---

AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------    ----------      -----------------------------------------
    sds		2 Dec 88	Initial Coding
		14 Dec		New strings
		19 Dec		now using PMACRO typedef
		20 Dec		added bCenterDlg
		30 Dec		added cxScreen, cyScreen
    sds		3 Jan 89	added szWinOldApp, globalized common strings
				  so won't reload (slowly) all the time
		10 Jan		added lpfnEnum
		1 Feb 89	added SysFontFace,size
		1 Mar 89	removed SysFontFace,size
		14 Mar 89	timer stuff
		15 Mar 89	DefFlags now a WORD
		16 Mar 89	added szErrArg
		25 Mar 89	added szErrResource
		28 Mar 1989	szTempDir
		30 Mar 1989	szNomem
		5 Apr 1989	bRecording
		27 Apr 1989	fBreakType, szDeskInt
		5 May 1989	szDeskInt deleted
		9 May 1989	hWndKey
		16 May 1989	bProtMode
		18 May 1989	REMOTECTL stuff
		18 May 1989	massive user intfc changes
		12 Jun 1989	bProtMode now bNotBanked, bNoFlash, WIN2 stuff
		27 Jul 1989	.hlp now loaded in szDothlp
		17 Aug 1989	fBreakType now a word
		18 Sep 1989	bSuspended
		26 Oct 1990	bBreakCheck
*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */

#include <windows.h>
#include <port1632.h>
#include "recordll.h"
#include "recorder.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */

/* VARIABLES DEFINED */
CHAR szRecorder[]="Recorder";	/* class name, icon name, WIN.INI section */
CHAR szPlayback[]="playback";	/* for WIN.INI */
CHAR szRecord[]="record";	/* for WIN.INI */

CHAR szNull[]="";
CHAR *szStarExt=NULL;
CHAR szDirSeparator[]="\\";
CHAR szScratch[CCHSCRATCH];
CHAR *rgszMsg[CMSGTEXT];
CHAR *szMouseCmd=NULL;
CHAR *szKeyCmd=NULL;
CHAR *szTempFile=NULL;
CHAR *szTempDir=NULL;
CHAR *szCaption=NULL;
CHAR *szDesktop=NULL;	/* class name for display */
CHAR *szDlgInt=NULL;	    /* #int class name */
CHAR *szDialogBox=NULL;     /* class name for display */
#ifdef WIN2
CHAR *szMsgInt=NULL;	    /* #int class name */
CHAR *szMessageBox=NULL;    /* class name for display */
CHAR *szWin200=NULL;
CHAR *szMSDOS=NULL;
#endif
CHAR *szWindows=NULL;
CHAR *szWinOldApp=NULL;
CHAR *szAlt=0;
CHAR *szControl=0;
CHAR *szShift=0;
CHAR *szScreen=NULL;
CHAR *szWindow=NULL;
CHAR *szNothing=NULL;
CHAR *szEverything=NULL;
CHAR *szClickDrag=NULL;
CHAR *szAnyWindow=NULL;
CHAR *szSameWindow=NULL;
CHAR *szFast=NULL;
CHAR *szRecSpeed=NULL;
CHAR *szErrResource=NULL;
CHAR *szErrArg=NULL;	/* set by whomever, for ErrMsgArg */
CHAR *szNoMem=NULL;
CHAR *szError=NULL;
CHAR *szWarning=NULL;
CHAR *szDotHlp=NULL;

HWND hWndKey;
HWND hWndRecorder;
HWND hWndPrevActive=0;
HWND hWndList;
HANDLE hMenuRecorder;
HANDLE hInstRecorder;
HANDLE hCurWait;
HANDLE hCurNormal;
WORD cxScreen;
WORD cyScreen;
FARPROC lpfnEnum;
#ifdef REMOTECTL
WORD wm_bridge=0;
BOOL bNoFlash=FALSE;
#endif

FARPROC lpfnWndProc;
WORD wTimerID=0;

BOOL bHotKeys=TRUE;
BOOL bFileChanged=FALSE;
BOOL bRecording=FALSE;
BOOL bMinimize=TRUE;
WORD fBreakType=0;
BOOL bSuspended=FALSE;
BOOL bNotBanked=FALSE;
BOOL bBreakCheck=TRUE;
BOOL bCenterDlg=FALSE;

/* current file data */
CHAR *szFileName=0;
CHAR *szUntitled=0;
OFSTRUCT ofCurrent;

#ifdef NTPORT
INT hFileIn=-1;
INT hFileOut=-1;
#else
HANDLE hFileIn=-1;
HANDLE hFileOut=-1;
#endif

PMACRO pmFirst=0;
PMACRO pmNew=0;
PMACRO pmEdit=0;

/* trace control flags */
BYTE MouseMode;
WORD DefFlags;
BYTE DefMouseMode;
struct pberror pbe;

/* FUNCTIONS REFERENCED */


/* VARIABLES REFERENCED */


/* -------------------------------------------------------------------- */

/* ------------------------------ EOF --------------------------------- */
