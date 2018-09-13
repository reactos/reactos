/***********************************************************************
NAME

    rdglob.c -- globals for recorder dynalink

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
    sds		5 Dec 88	Initial Coding
		14 Dec		Added bStartUp
		15 Dec		RecFlags now a word
		30 Dec		added screen size support
		3 Jan 89	LastDown now global
		6 Jan		renamed to bHaveMouseMsg
		12 Jan		changed some BOOL's to BBOOL
		16 May 1989	bProtMode
		9 Jun 1989	name change to recorder
    sds 	12 Jun 1989	bProtMode now bNotBanked
    sds 	27 Nov 1990	sysmsg moved here
*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */
#include <windows.h>
#include <port1632.h>
#include "recordll.h"
#include "msgs.h"
#include "rdlocal.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */

/* VARIABLES DEFINED */

struct hotkey *phBase=0;
INT cHotKeys=0;
BBOOL bSuspend=FALSE;
BBOOL bNoHotKeys=FALSE;
BBOOL bBreakCheck=TRUE;
BBOOL bNotBanked=FALSE;
CHAR cButtonsDown=0;	/* number of buttons down */
LONG StartTime=0;
FARPROC lpOldKHook=0;
FARPROC lpOldJRHook;
DWORD DblClickTime;
struct btimes LastDown={0};

/* for record mode */
EVENTMSG sysmsg;
struct stringlist *psFirst = 0;	/* window names during record */
BBOOL bRecording=FALSE;
BBOOL bStartUp=FALSE;
INT MouseMode;
WORD cMsgs;
HANDLE hMemTrace;
WORD RecFlags;
WORD cxScreen;	    /* system metrics */
WORD cyScreen;
BBOOL bHaveMouseMsg=FALSE;

/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */

/* ------------------------------ EOF --------------------------------- */
