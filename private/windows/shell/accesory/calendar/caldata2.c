/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** caldata2.c
 *****
*/

/* Get rid of more stuff from windows.h */
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOSYSCOMMANDS
#define NOMEMMGR
#define NOCLIPBOARD
#define NOVIRTUALKEYCODES

#include "cal.h"


CHAR *vrgsz [CSTRINGS+1];  /* Strings loaded in from the resource file. */

D3   vd3Sel;		 /* Selected date. */
WORD vwDaySticky;	 /* The sticky selected day. */

INT vrgbMonth [CBMONTHARRAY];	  /* Days of the month being displayed.
				     Changed from BYTE to int  */
INT  vcDaysMonth;	 /* Number of days in the month being displayed. */
INT  vcWeeksMonth;	 /* Number of weeks in the month being displayed. */
WORD vwWeekdayFirst;	 /* The weekday of the first day of the month. */

INT  vrgxcoGrid [8];	 /* Month grid xco - maximum of 7 days, so 8 lines. */
INT  vrgycoGrid [7];	 /* Month grid yco - maximum of 6 weeks, so 7 lines. */

WORD votqrPrev;
WORD votqrCur;
WORD votqrNext;

WORD vidrCur;		      /* The index into vrghlmdr for the current DR. */
LOCALHANDLE vrghlmDr [CDR];   /* The handles of the DRs. */

LOCALHANDLE  vhlmTdd = 0; /* Handle to table of date descriptors. */
INT  vcddAllocated = 0;	  /* Count of allocated date descriptors. */
INT  vcddUsed = 0;		  /* Count of date descriptors in use. */

/* Used for passing dates from dialogs to command handlers. */
DT   vdtFrom;
D3   vd3To;
DT   vdtTo;
INT  vitddFirst;
INT  vitddMax;

/* The handle of the window that should get the focus when we get
   activated via a WM_ACTIVATE with wParam == TRUE.  When the
   appointment description edit control or the notes area
   edit control notifies us that it is getting the focus, we store
   its window handle in vhwndFocus.  When we set focus to the monthly
   calendar, we also set up vhwndFocus.
*/
HWND vhwndFocus = (HWND)NULL;


/* File handles. */
INT  hFile[CFILE];

/* ReOpen Buffers. */
OFSTRUCT  OFStruct [CFILE];

BOOL vfChangeFile = FALSE;    /* FALSE if couldn't create the change file,
				 TRUE if change file exists.
				 Must be initialized to FALSE so the first
				 call to CreateChangeFile doesn't attempt
				 to delete an old change file (since there
				 isn't one).
			      */
INT vobkEODChange;	 /* Offset in blocks of the end of data in the
			    change file.
			 */

CHAR vszFileSpec   [CCHFILESPECMAX];	/* Name of original file. */
CHAR vszFilterSpec [CCHFILTERMAX];	/* Filter string for File Open */
CHAR vszCustFilterSpec [CCHFILTERMAX];/* custom Filter string for File Open */

BOOL vfOriginalFile;	      /* FALSE means untitled, TRUE means there
				 is an original file.
			      */
INT  vobkEODNew;	 /* Offset in blocks of the end of data in the
			    new file.
			 */

BOOL vfDirty;		 /* FALSE means no changes since the last Save. */

/* This is the magic number we put at the beginning of a calendar file
   so no one fools us into using some other sort of file.
*/
BYTE vrgbMagic [CBMAGIC] =
     {
     'C' + 'r',
     'A' + 'a',
     'L' + 'd',
     'E' + 'n',
     'N' + 'e',
     'D' + 'l',
     'A' + 'a',
     'R' + 'c'
     };

HDC vhDCMemory = 0;		  /* Memory DC for BitBlts. */
HBITMAP vhbmLeftArrow = 0;	  /* Handle to bitmap for left arrow. */
HBITMAP vhbmRightArrow = 0;	  /* Handle to bitmap for left arrow. */
HBITMAP vhbmBell = 0;		  /* Handle to alarm bell bitmap. */

/* x coordinates within Wnd2A of the day switching arrows. */
INT  vxcoLeftArrowFirst;
INT  vxcoLeftArrowMax;
INT  vxcoRightArrowFirst;
INT  vxcoRightArrowMax;


/* Merge spec - two byte string */
//- Merge Bytes: This is used to grab two chars from a string, make it string.
char vszMergeStr [3];

INT vmScrollPos=0;  /* Thumb position for vertical month scroll */
INT vmScrollInc=0;  /* step size for vertical month scroll */
INT vmScrollMax=0;  /* maximum vertical scroll position */
INT hmScrollPos=0;  /* Thumb position for horiz. month scroll */
INT hmScrollInc=0;  /* step size for horiz. month scroll */
INT hmScrollMax=0;  /* maximum horiz. scroll position */
