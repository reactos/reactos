/*
 *   Windows Calendar
 *   Copyright (c) 1985 by Microsoft Corporation, all rights reserved.
 *   Written by Mark L. Chamberlin, consultant to Microsoft.
 *
*/

/*
 *****
 ***** caldata.c
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

// While processing command line options, there may be errors
// (e.g. can't open file) which need to be put up using AlertBox(). Once
// alert box completes, the focus is set to the parent window which results
// in WM_ACTIVATE. If WM_ACTIVATE is processed before a file is loaded
// it results in GP Faults. 
// Prevent processing WM_ACTIVATE when initialization is not yet complete
// Set after CalInit() completes. 
BYTE fInitComplete = FALSE;

BYTE vrgcDaysMonth [12] =
     {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

BOOL vfDayMode = FALSE;  /* TRUE if in day mode, FALSE if in month mode.
                            Note - it's important that be initialize to
                            FALSE so the first call to DayMode () sets
                            everything up.
                         */

HANDLE vhInstance;       /* Current instance handle. */

HBRUSH vhbrBorder = 0;       /* Brush to use for borders - this uses the system
                            window borders color.
                         */
HBRUSH vhbrBackMain = 0;     /* Background brush for the main window - this
                            uses a fixed color defined by Tandy.
                         */
HBRUSH vhbrBackSub = 0;      /* Background brush for all sub windows - this uses
                            the system window background color.
                         */

HCURSOR vhcsrArrow;
HCURSOR vhcsrIbeam;
HCURSOR vhcsrWait;

INT  vcxFont;		 /* Width of fixed font. */
INT  vcxFontMax;	 /* Maximum font width */
INT  vcyFont;            /* Height of fixed font. */
INT  vcyDescent;         /* The descent of the fixed font. */
INT  vcyExtLead;         /* External leading of fixed font.  Pronounced
                            as in the metal lead, this is the blank space
                            separating two lines of text.  This space "belongs"
                            to the first of two lines (i.e., it follows the
                            text rather than preceding it).
                         */
INT  vcyLineToLine;      /* Base line to base line of fixed font
                            (Same as vcyFont + vcyExtLead).
                         */

INT  vcxBorder;
INT  vcxVScrollBar;
INT  vcyBorder;
INT  vcxHScrollBar;
INT  vcyHScrollBar;

/* Window heights.  Note that all heights are  of client areas except for
   vcyWnd1, which includes the top and bottom borders.
*/
INT  vcyWnd1;
INT  vcyWnd2A;
INT  vcyWnd2BTop;
INT  vcyWnd2BBot;
INT  vcyWnd2B;

/* Window widths.  Note that all widths are  of client areas except for
   vcxWnd1, which includes the left and right borders.
*/
INT  vcxWnd1;
INT  vcxWnd2A;
INT  vcxWnd2B;

INT  vxcoBell;           /* Where alarm bell bitmap starts. */
INT  vcxBell;            /* Width of the alarm bell bitmap. */
INT  vcyBell;            /* Height of the alarm bell bitmap. */
INT  vxcoApptTime;       /* Where the appointment time starts. */
INT  vxcoAmPm;           /* Where the am or pm string starts. */

/* The limits of the appointment description portion of wnd2B. */
INT  vxcoQdFirst;
INT  vxcoQdMax;
INT  vycoQdFirst;
INT  vycoQdMax;

INT  vxcoDate; /* Where the date begins in wnd2A. */

/* The bottom box of the calendar encloses the notes edit control, but the
   edit control does not take up the entire box.  We draw a double line
   above the notes.  The upper line is in wnd2B, the lower is drawn
   in wnd1 at vycoNotesBox.
*/
INT  vycoNotesBox;

/* Coordinates of the notes edit control within wnd1. */
INT  vxcoWnd2C;
INT  vycoWnd2C;

INT  vcln;     /*** ln - a line in the appointments window (0 through
                    vcln - 1).
               */
INT  vlnLast;  /* The highest line number. */

LD vtld [25];       /* The table of line descriptors. */
/* Window handles. */
HWND vhwnd0;
HWND vhwnd1;
HWND vhwnd2A;
HWND vhwnd2B;
HWND vhwnd2C;
HWND vhwnd3;
HWND hEditHide;
#ifndef BUG_8560
HWND vhScrollWnd;	/* Window handle of the scrollbar handle */
#endif

D3   vd3Cur;                       /* Date when we last called ReadClock. */
FT   vftCur;                       /* Date and time when we last called
                                      ReadClock.
                                   */

WORD vcMinEarlyRing;               /* Alarm early ring period. */
BOOL vfSound;                      /* TRUE if the audio alarm is enabled. */
BOOL vfHour24;                     /* TRUE for 24 hour format, FALSE for
                                      12 hour format.
                                   */
INT  vmdInterval;                  /* Interval between appointments:
                                      MDINTERVAL15, MDINTERVAL30, or
                                      MDINTERVAL60.
				   */

INT viMarkSymbol    = 0;
BOOL vfOpenFileReadOnly = FALSE;

INT  vcMinInterval;		   /* Interval stored in minutes. */
TM   vtmStart;                     /* Starting time of day mode. */

FARPROC vrglpfnDialog [CIDD] =
     {
     (FARPROC)0,  /* FnSaveAs, removed by L.Raman 12/12/90 (common dlg. support) */
     (FARPROC)FnPrint,
     (FARPROC)FnRemove,
     (FARPROC)FnDate,
     (FARPROC)FnControls,
     (FARPROC)FnSpecialTime,
     (FARPROC)FnDaySettings,
     (FARPROC)0,    // FnAbout, removed by TG 12/17/90 (ShellAbout support)
     (FARPROC)FnAckAlarms,
     (FARPROC)FnPageSetup
     };

FARPROC lpfnMark;

INT  vlnCur;		      /* The current ln. */

FT   vftAlarmNext;            /* The next alarm to go off.  vftAlarmNext.dt
                                 == DTNIL means there is no next alarm.
                              */
FT   vftAlarmFirst;           /* The first unacknowledged alarm.
                                 vftAlarmFirst.dt == DTNIL means there are
                                 no unacknowledged alarms.
                              */
BOOL vfFlashing = FALSE;      /* If TRUE, we are flashing the window (title
                                 bar or icon to tell the user to make us
                                 the active window.
                              */
INT  vcAlarmBeeps = 0;        /* The number of beeps remaining for the
                                 alarm.
                              */

BOOL vfInsert;                /* Only valid during a special time command.
                                 FALSE means Delete, TRUE means insert.
                              */
TM   vtmSpecial;              /* Only valid during a special time command.
                                 The tm to delete or insert.
                              */

BOOL vfNoGrabFocus;           /* If Calendar is brought up iconic this flag
                                 gets set by CalInit in order to prevent the
                                 focus from being grabbed.  Instead of calling
                                 SetFocus directly, all routines call
                                 CalSetFocus, which only calls SetFocus
                                 if vfNoGrabFocus is FALSE.
                              */

HANDLE vhAccel;               /* Handle to the accelerator table. */
INT viAMorPM = IDCN_AM;

/* AlertBox needs to know what window is the parent of the message box.
   If there is a dialog in progress, the parent is the dialog box.
   Otherwise, the parent window is Calendar's tiled window.
   In order to avoid having to pass down the parent window handle
   through numerous levels of calls, we simply remember the window
   handle of the active dialog in vhwndDialog.  It is initially set
   to (HWND)NULL and gets reset to this at the end of FDoDialog.
   Each dialog stores it's window handle into vhwndDialog when it
   handles the WM_INITDIALOG.  Note that there are no cases of nested
   dialogs, so having a single global works.
   (As an example of where the parent window handle would have to be
   passed down numerous levels, consider the FSaveFile case.  It can
   be called while the SaveAs dialog is up, or from Save (in which
   case there is no dialog.  It in turn goes several layers deep
   and gets into routines like FWriteFile which can call AlertBox to
   report a Disk Full error.  So using a global makes a lot of sense.)
*/
HWND vhwndDialog = (HWND)NULL;

/* If the system clock is set back by one or more minutes, or it is
   set ahead by 1440 (the number of minutes in a day) or more minutes,
   this flag gets set so we know we have to resynchronize the next alarm.
*/
BOOL vfMustSyncAlarm = FALSE;

/* globals for file page setup and file print */

CHAR chPageText[6][PT_LEN];
CHAR szDec[5];
CHAR szPrinter[128];
BOOL bPrinterSetupDone=FALSE;

INT viLeftMarginLen;      /* page left margin length   */
INT viRightMarginLen;     /* page right margin length  */
INT viTopMarginLen;       /* page top margin length    */
INT viBotMarginLen;       /* page bottom margin length */

INT viCurrentPage = 0;		    /* current page being printed */

OPENFILENAME vOFN;		    /* struct. for the common
				     * file open and saveas dialogs
				     */
INT vFilterIndex = 1;		    /* default filter index in File/Open
				     * dialog
				     */
PRINTDLG vPD;			    /* struct. passed into PrintDlg */
INT vHlpMsg;			    /* message nummer to invoke Help appl. from
				     * common dialogs
				     */
