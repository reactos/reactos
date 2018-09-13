/************************************************************************
NAME

    recordll.c -- dynalink for recorder applet

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
		9 Dec		No hot keys while recording no-nest macro
				  always record module/class, even if
				  !PlaytoWindow
				Fixed record bug for Everything mode
				Rearranged tRecordEvent to realloc segment
				  BEFORE new message added
				MouseMove with no capture or buttons
				  no longer complains about wrong active
				  window.
		12 Dec		Special cased pb to desktop
				More playback fixes
		13 Dec		Yet more PB tweaks
		14 Dec		ditto
				RecordEvent now trashes everything before
				 1st keyup or buttonup (for Record dlg box)
				Now do pause if pb mouse to invisible
				 window (not painted yet)
				Now lift mouse buttons on PB error
		15 Dec		Now deals with capslock better
		19 Dec		cMsgs now in paramL of 1st msg
				autoloop now supported
		20 Dec		fixed bug allowing ipb to go < -1
				added dblclick handling
		21 Dec		fixed dblclick
		30 Dec		now trashing trapped ctrl/break
    sds		4 Jan 89	numerous efficiency improvements
				Fixed bogus RECSPEED timing - now takes
				  actual playback time into account
				Fixed bug regarding right or middle button
				  up - not necessarily active window
    sds		6 Jan		renamed to bHaveMouseMsg
				static'ed some internals
				now times out on PB to invisible window
				KHook now trashes EVERYTHING on a ctrl/break
				  until CANCEL UP key
				now stuffing WM_MOUSEMOVE after every
				  non-MOUSEMOVE JP msg, with our own MousePos
    sds		9 Jan		Fixed mouse move stuffing.
				Imposed timeout on kbd break eating, in
				  case other app took CANCEL key
				RecordEvent now has active window override
				  WindowFromPoint to handle case where it
				  hasn't displayed yet
				Now using PBMessage() and forcing PB of
				  same message on duplicate HC_GETNEXT calls
				Now setting/clearing ButtonFlags AFTER
				  we are sure msg is actually going out
		10 Jan		FirstPBMouse now stuffs WM_MOUSEMOVE
				some efficiency improvements
		11 Jan		IsWindowVisible PB Check is now tempered
				  by !IsIconic
				bHaveMouseMsg now done in Heuristics
		12 Jan		changed some BOOL's to BBOOL
				Now fudging WM_MOUSEMOVE any time we
				 output a mouse msg where CursorPos not
				 already there.
		13 Jan		Now stomping time in first recorded msg to 0
    sds		28 Feb 89	now using UnhookWindowsHook properly
    sds		1 Mar 89	now enforcing delay to avoid bogus dbl click
				  only if was NOT a dbl click at record time
				now using TranslateMessage instead of
				  ToAscii to update keyboard lights
    sds		2 Mar 89	now avoiding expensive SetCursorPos call
				  unless really necessary
    sds		3 Mar 89	now timestamping JP events in the PAST
				  if record-time interval indicates
				  it.  Preserves dbl-clicks better
    sds		15 Mar 89	expanded MAXWAITPAINT for invisible window
    sds		16 Mar 89	hWndTarget at PB now ignores capture for
				  key msgs
    sds		27 Mar 1989	now handling HC_SYSMODALx
				bugfix where Heuristics deletes everything
    sds		30 Mar 1989	added workaround for Windows bug in trashing
				prevkeystate bit on KeyHook(HC_NOREM)
    sds		3 Apr 1989	[SYS]KEYUP's now playback to ANYBODY
    sds		4 Apr 1989	improved KEYUP playback
    sds		6 Apr 1989	RecordEvent now private
    sds		26 Apr 1989	added B_FORCEACTIVE bit to buttondown
				msg following junked alt/tab/esc sequence
    sds		27 Apr 1989	sysmodal dlgbox on record now suspends
				szDesktop is now #32769
				now using windows lstrcmp
    sds		3 May 1989	win2 support fix
    sds		5 May 1989	now using GetDesktopHwnd
    sds		9 May 1989	changed check for msg to Tracer
    sds		13 May 1989	fixed bug in recording event timing
    sds		16 May 1989	no NOT_BANKED memory in protect mode
				renamed from tdyn.c
    sds		22 May 1989	pb mouseUP to overlapped window now OK
    sds		23 May 1989	now notifies Tracer on pb done
    sds		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	bProtMode now bNotBanked
    sds		28 Jul 1989	BreakPlayback for REMOTECTL
    sds		7 Aug 1989	increased MAXWAITPAINT, sometimes they really
				wail on JPHook without painting anything
    sds		8 Aug 1989	now passing event ptr on WM_JPACTIVATE, for
				  expanded wait-for-window-to-appear
    sds		17 Aug 1989	bug fix - now detecting nested pb via
				 id, not unreliable hMem
				now allows save of macro on TOOLONG error
    sds		25 Aug 1989	removed "include winexp.h", in windows.h now
    sds		19 Sep 1989	now using PostMessage(LBUTTONDOWN) to
				desktop to clear menu mode
    sds		20 Sep 1989	WIN2 stuff, winexp.h needed
    sds		10 Oct 1989	bug fix handling no-break check option
    sds		27 Oct 1989	now handles bizarre case of finding
				  playback target window, yet it is
				  destroyed before next JPHook call
    sds		31 Oct 1989	fixed innocuous bug in AddString
    sds		26 Dec 1989	no longer produces PB errors for mouse MOVE
				  to obscured window if no buttons are down
				added error detection for mouse off screen
    sds		8 Jan 1990	now detects failure to auto-activate window
				  on playback
    sds		19 Jan 1990	put in extra test on validity of hWndActivate
    sds		5 Feb 1990	WIN2 fix
    sds		16 Feb 1990	workaround for out-of-sequence sysqueue msgs
    sds		18 Feb 1990	now detects break during long playback
				  delays
				now plays back KEYUP's for shift/control/alt
				  at playback termination, in same manner
				  as button up
    sds		20 Feb 1990	now gives up and skips anyway on MAXDUP
				  consecutive duplicate HC_GETNEXT
    sds		21 Feb 1990	patched up tiny possibility of skipping
				  a real msg on MAXDUP stuff from yesterday
    sds		28 Feb 1990	now doing more complete resynch of kbd
				  and async kbd tables on pb termination
				now handles case where window is a child
				  of desktop
    sds		7 Mar 1990	Win2.1 code fixes
    sds		15 Mar 1990	Win2.1 code fixes
    sds		26 Oct 1990	StopPlayback now takes bWasBreak from record.c
    sds		27 Nov 1990	sysmsg moved to rdlocal.c
    sds		5 Dec 1990	now using AnsiNext to walk char ptr
    sds		8 Jan 1991	fixes for MSC6, fixed bug with combobox's
				  listbox as playback target
*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */
#include <windows.h>
#include <port1632.h>
#include <string.h>
#ifdef WIN2
#include <winexp.h>
#endif
#include "recordll.h"
#include "msgs.h"
#include "rdlocal.h"


/* PREPROCESSOR DEFINITIONS */
#define MAXNEST	5	/* max nested playbacks */
#define MAXWAITPAINT 20	/* JP calls to wait for not-visible window to paint
			    (10 fails sometimes) */
#define MAXEATTIME 2000L    /* max msec to eat keys on Break detection */
#define MAXDUP	10	/* max times playing back same msg */
#define CCHSCRATCH 255
#define MAKEWORD(l,h) ((WORD) (((BYTE)(l)) | (((BYTE)(h)) << 8)))

#ifdef WIN2
#define HC_SYSMODALON	-100	/* don't exist in Win2.1 */
#define HC_SYSMODALOFF	-100
#endif

#define BUT_LEFT    1
#define BUT_MIDDLE  2
#define BUT_RIGHT   4
#define K_ALT	    8
#define K_CONTROL   0x10
#define K_SHIFT	    0x20

/* FUNCTIONS DEFINED */

static struct stringlist *AddString(CHAR *sz);
static BOOL NEAR Break(VOID);
static BOOL NEAR CheckOverlap(LPEVENTMSGMSG lpMsg,HWND hWndTarget);
static BOOL NEAR CheckShift(INT flags,INT mask);
static INT NEAR ForceActiveFlag(WORD);
VOID NEAR InstallJP(VOID);
static BOOL NEAR IsBenignMove(LPEVENTMSGMSG lpMsg);
static BOOL NEAR IsMouseUp(WORD);
static BOOL NEAR IsOwnedBy(HWND hWndOwner,HWND hWnd);
FARPROC APIENTRY JPHook(INT nCode,WPARAM wParam,LPEVENTMSGMSG lpMsg);
static VOID NEAR GetModClass(HWND hWnd);
static HWND NEAR GetPopParent(HWND hWnd);
static BOOL NEAR InBounds(LPEVENTMSGMSG lpMsg,LPRECT lpRect);

LONG NEAR JPError(WORD wParam, WORD offset);

#ifdef NTPORT
HWND GetForegroundWindow( VOID );
BOOL APIENTRY REnumProc( HWND, LONG );
#endif

VOID NEAR JRError(WORD ecode);
static LONG NEAR lmin(LONG,LONG);
static LPSTR lstrrchr(LPSTR lpsz,UCHAR c);
static BOOL NEAR Nested(BYTE id);
FARPROC NEAR PBMessage(LPEVENTMSGMSG lpMsg,BOOL bNew);
static VOID NEAR RecordEvent(PEVENTMSGMSG pMsg);

/* VARIABLES DEFINED */
CHAR szScratch[CCHSCRATCH];

#ifdef WIN2
/* IMPORTANT - these strings really should come from Recorder .RC file,
    but hard to do while this is still a dynalink.  Get to it later */
CHAR szWinOldAp[]="WinOldApp";
CHAR szDesktop[]="desktop";
#endif

BYTE WaitPaint=0;

/* scan codes used for stuffing keyup msgs - set at playback time */
CHAR scanAlt=0;
CHAR scanShift=0;
CHAR scanControl=0;

CHAR szClassName[100];
LPSTR lpszCls=&szClassName[0];

HWND hWndActivate=(HWND)-1;  /* hWnd for auto activation in playback */
FARPROC lpOldJPHook;
EVENTMSG emDup; 	     /* stores msg for duplicate HC_GETNEXT calls */
BBOOL bDupMsg=FALSE;	     /* TRUE if duplicate HC_GETNEXT call */
BYTE cDups=0;		     /* number of times dup message was played */
BYTE ForcedSkip=0;	    /* 0 normally, 1 if forcing an HC_SKIP, 2 if prev
				call was a HC_GETNEXT forced to HC_SKIP */

HWND hWndRecorder=0;
HWND hWndDesktop=0;	/* remember it for speed */
DWORD DelayTill=0;	/* time to stop delays on JPHook, in msec */
DWORD PrevMsgTime=0;	/* GetTickCount time in msec of last PB message */
BBOOL bWait=FALSE;	/* TRUE for delay until Hotkey PostMessage takes */
CHAR ButtonFlags=0;	/* which ones */

#ifdef REMOTECTL
BBOOL bBreakPB=FALSE;  /* TRUE to break playback */
#endif

DWORD EatTimeout=0L;	/* when to stop eating keys, even if BREAK not seen */
BBOOL bEatBreak=FALSE;	/* TRUE to discard break after async detection */
BBOOL bAbortMode=FALSE;	/* TRUE if lifting buttons on PB abort */
BBOOL bFudgeMouse=FALSE;	/* TRUE if must kludge MOUSEMOVE message */
BBOOL bFudged=FALSE;	/* TRUE if just did kludge MOUSEMOVE */
POINT MouseLoc;		/* our idea of where mouse should be during PB */

#ifdef NTPORT
HHOOK hhkPlay;		/* Handle to the keyboard hook */
HHOOK hhkRecord;	/* Handle to the Journal Record hook */
HHOOK hhkKey;		/* Handle to the Journal Playback hook */
#endif

INT ipb=-1;	/* current playback index */

struct btimes TimeSince;    /* time since last button, in pb stream */

#ifdef NTPORT
struct PlayBack {
    HANDLE hMem;	       /* of playback segment */
    LPRECORDERMSG lptS;        /* ptr to the beginning of the memory block */
    LPRECORDERMSG lptCurrent;  /* ptr to current msg in memory block */
    INT cMsgs;		       /* remaining in playback */
    BYTE flags;
    BYTE id;		       /* identifier needed for error reporting */
    CHAR rgchKB[256];	       /* save kb state to restore after playback */
} pbStack[MAXNEST];
struct PlayBack *pb;	       /* current playback pointer */
#else
struct PlayBack {
    HANDLE hMem;	    /* of playback segment */
    LPRECORDERMSG lptCurrent;   /* ptr to current msg in segment */
    INT cMsgs;		    /* remaining in playback */
    BYTE flags;
    BYTE id;		    /* identifier needed for error reporting */
    CHAR rgchKB[256];	/* save kb state to restore after playback */
} pbStack[MAXNEST];
struct PlayBack *pb;	/* current playback pointer */
#endif

struct {
    WORD msg;
    WORD wParam;
    LONG lParam;
} jpe;	    /* holding area for playback error/abort PostMessage values */

/* FUNCTIONS REFERENCED */

#ifdef NTPORT
WORD Dif(LPRECORDERMSG, LPRECORDERMSG);
#else
VOID APIENTRY lmemset(LPVOID,CHAR,INT);
#endif

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */

/*************************************
    Externally visible functions
*************************************/

#ifdef REMOTECTL
VOID APIENTRY BreakPlayback()
{
    if (ipb > 0)
	bBreakPB = TRUE;
}
#endif

INT APIENTRY StopRecord(struct macro FAR *lpm,BOOL bKeepIt)

/* ALWAYS called when record process is completed (or aborted) */
/* WARNING: lpm may be NULL */
{
    LPRECORDERMSG lpt;
    LPSTR lpch;
    INT i;
    BOOL b;
    register struct stringlist *ps;
    HANDLE hMem;
    WORD cchSeg,cch;

    if (!bRecording)
	/* just a blunderbus remove hooks call */
	return 0;

    if (!bSuspend)
	/* no ctrl/break, so hook is still there - clear it */
#ifdef NTPORT
	UnhookWindowsHookEx(hhkRecord);
#else
	UnhookWindowsHook(WH_JOURNALRECORD,(FARPROC) JRHook);
#endif
    bSuspend = bRecording = FALSE;

    while (bKeepIt && cMsgs > 1) {
	/* (this isn't really a while loop, but I needed to be able to
	    break out if Heuristics throws whole macro away) */
	if (i = Heuristics()) {
	    GlobalFree(hMemTrace);
	    hMemTrace = 0;
	    return i;
	}
	if (cMsgs <= 1)
	    break;

	if (bHaveMouseMsg) {
	    lpm->f.cxScreen = cxScreen;
	    lpm->f.cyScreen = cyScreen;
	}
	else
	    lpm->f.cxScreen = 0;

	lpt = (LPRECORDERMSG) GlobalLock(hMemTrace);
	/* first paramL contains msg count */
	lpt->paramL = cMsgs;
	/* first real msg delay always 0 */
	(lpt + 1)->time = 0;
	GlobalUnlock(hMemTrace);

	/* add strings to end of segment and fix up offsets */
	cchSeg = cMsgs * sizeof(RECORDERMSG);
	for (ps = psFirst; ps; ps = ps->psNext) {
	    /* fix all ptrs to this string */
	    b = FALSE;
	    lpch = GlobalLock(hMemTrace);
	    for (lpt = (LPRECORDERMSG) lpch, i = cMsgs; i > 0;
	      i--, lpt++)
		if (lpt->u.ps == ps) {
#ifdef NTPORT
		    lpt->u.szWindow = (UINT)cchSeg;
#else
		    lpt->u.szWindow = (CHAR *) cchSeg;
#endif
		    b = TRUE;
		}
	    GlobalUnlock(hMemTrace);
	    if (!b)
		/* string became redundant in Heuristics */
		continue;

	    /* put string into macro code segment */
	    cch = strlen(ps->sz) + 1;

	    if ((hMem = GlobalReAlloc( hMemTrace,
				      (LONG)cchSeg+cch,
				       GMEM_MOVEABLE|GMEM_NOT_BANKED|GMEM_SHARE ))==0){
		GlobalFree(hMemTrace);
		hMemTrace = 0;
		return E_NOMEM;
	    }
	    lpch = GlobalLock(hMemTrace = hMem);
	    lmemcpy(lpch + cchSeg,ps->sz,cch);

	    cchSeg += cch;
	    GlobalUnlock(hMemTrace);
	}
	FreeStringList();
	lpm->f.cchCode = cchSeg;
	lpm->hMemCode = hMemTrace;
	hMemTrace = 0;
	return 0;
    }

    /* not keeping macro, or there were no events */

    /* free trace block here - Recorder has no hMem for it */
    GlobalFree(hMemTrace);
    hMemTrace = 0;
    return (bKeepIt ? M_NOEVENTS : 0);
}

INT APIENTRY StartPlayback(HANDLE hMem,BYTE id)
/* hMem must be unbanked, id is a unique identifier */
{
    CHAR rgchKB[256];

#ifdef NTPORT
    INT  i;
#endif

#ifdef NTPORT
OutputDebugString("START: StartPlayback\n\r");
#endif

    if (ipb + 1 >= MAXNEST)
	return E_TOONESTED;
    if (Nested(id))
	/* never ever ever let same playback nest */
	return E_NESTED;
    if (++ipb == 0)
	/* first playback */
	InstallJP();
/* we probably should unlock old playback and keep only the current one
locked, but since playbacks are always wired in low memory anyway
it doesn't make a difference yet.  Should be more clever. */
    pb = &pbStack[ipb];
    pb->lptCurrent = (LPRECORDERMSG) GlobalLock(pb->hMem = hMem);

#ifdef NTPORT
    pb->lptS=pb->lptCurrent;   /* keep track of memory block start */
#endif

    /* pick up msg count from first RECORDERMSG */
    pb->cMsgs = pb->lptCurrent->paramL - 1;
    pb->flags = (BYTE) pb->lptCurrent->message;
    pb->id = id;
    /* save keyboard state */
    GetKeyboardState(pb->rgchKB);

    /* ain't no keys down yet */
#ifdef NTPORT
    for(i=0;i<256;i++) rgchKB[i]='\0';
#else
    lmemset(rgchKB,'\0',256);
#endif

    /* except what was there at record time */
    if (pb->lptCurrent->message & B_CAPSLOCK)
	rgchKB[VK_CAPITAL] |= 1;
    if (pb->lptCurrent->message & B_NUMLOCK)
	rgchKB[VK_NUMLOCK] |= 1;
    if (pb->lptCurrent->message & B_SCRLOCK)
	rgchKB[VK_SCROLLLOCK] |= 1;
    SetKeyboardState((LPSTR ) rgchKB);

    /* point to first real msg */
    pb->lptCurrent++;

    /* enforce at least a dblclick time between clicks in different
       macros */
    TimeSince.lRight = TimeSince.lMiddle = TimeSince.lLeft
      = (LONG) DblClickTime;
    return 0;
}

VOID NEAR InstallJP()
{

#ifdef NTPORT
OutputDebugString("START: InstallJP\n\r");
#endif

#ifdef NTPORT
    lpOldJPHook = SetWindowsHookEx(GetModuleHandle("recordll"),0,WH_JOURNALPLAYBACK,JPHook);
    hhkPlay	= lpOldJPHook;
#else
    lpOldJPHook = SetWindowsHook(WH_JOURNALPLAYBACK,(FARPROC) JPHook);
#endif

    cButtonsDown = ButtonFlags = 0;
    WaitPaint = 0;
    PrevMsgTime = GetTickCount();
    bDupMsg = bFudged = bAbortMode = bFudgeMouse = FALSE;
    hWndActivate =(HWND)-1;
    /* clear CANCEL key bit */
    GetAsyncKeyState(VK_CANCEL);
    GetCursorPos(&MouseLoc);
#ifndef WIN2
    hWndDesktop = GetDesktopWindow();

#ifdef NTPORT
 //   if (GetForegroundWindow())
    if (GetCapture())
#else
    if (GetCapture())
#endif
	/* somebody has capture, may be in menu mode - this blows us
	   out reliably */
	PostMessage(hWndDesktop,WM_LBUTTONDOWN,0,0L);
#endif
}

VOID APIENTRY SetPBWindow(HWND hWnd)
/* called from Recorder after a WM_JPACTIVATE msg was sent.  hWnd is that
    of target app, or NULL if not found */
{

#ifdef NTPORT
OutputDebugString("START: SetPBWindow\n\r");
#endif

    if (bWait) {
	hWndActivate = hWnd;
	Wait(FALSE);
    }
}

VOID APIENTRY StopPlayback(BOOL bAbort,BOOL bWasBreak)
{
    MSG msg;
    SHORT i;

#ifdef NTPORT
OutputDebugString("START: StopPlayback\n\r");
//DebugBreak();
#endif


    if (ipb >= 0) {
	if (bAbort) {
	    while (ipb > 0) {
		/* release memory */
		GlobalUnlock(pb->hMem);
		if (!bNotBanked)
		    GlobalFree(pb->hMem);
		pb = &pbStack[--ipb];
	    }
	}
	bWait = FALSE;	/* nothing to wait for now */
	if (bWasBreak && !bEatBreak) {
	    /* detected break through GetAsyncKeyState at main app level */
	    /* be sure to trash events at KeyHook */
	    bEatBreak = TRUE;
	    EatTimeout = GetTickCount() + MAXEATTIME;
	}
	ipb--;
	GlobalUnlock(pb->hMem);
	/* hMem needs to be freed below, unless B_LOOP */

	if (ipb < 0 || bAbort) {
	    /* clear semaphore on capslock */
	    GetAsyncKeyState(VK_CAPITAL);
	    for (i = 0; i < 256; i++) {
		if ((i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9') ||
		  (i >= VK_F1 && i <= VK_F16) ||
		  (i >= VK_NUMPAD0 && i <= VK_NUMPAD9))
		    /* clear out all the 'safe' keys - we definitely don't
		       want them restored down.  We must leave CAPSLOCK,
		       NUMLOCK, SCROLLLOCK, and perhaps unknown foreign shift
		       keys intact. */
		    pb->rgchKB[i] = '\0';
		else if (i == VK_SHIFT || i == VK_MENU || i == VK_CONTROL)
		    /* leave toggle alone, but definitely clear DOWN bit */
		    pb->rgchKB[i] &= 0x7f;

		if (GetAsyncKeyState(i) & 0x8000)
		    /* this key is really down - reflect it */
		    pb->rgchKB[i] |= 0x80;
	    }
	    SetKeyboardState(pb->rgchKB);

	    /* this forces a call to ToAscii, which updates the
		keyboard lights to agree with SetKeyboardState */
	    msg.hwnd = hWndRecorder;
	    msg.message = WM_KEYDOWN;
	    msg.wParam = VK_F8;
	    msg.lParam = 0;
	    TranslateMessage(&msg);

#ifdef NTPORT
	    UnhookWindowsHookEx(hhkPlay);
#else
	    UnhookWindowsHook(WH_JOURNALPLAYBACK,(FARPROC) JPHook);
#endif
	}
	else
	    /* restore kbd for popped macro */
	    SetKeyboardState(pb->rgchKB);

	if (!bAbort && (pb->flags & B_LOOP))
	    /* play it again, sam */
	    StartPlayback(pb->hMem, pb->id);
	else {
	    /* let Recorder know we are done with code */
	    PostMessage(hWndRecorder,WM_PBDONE,pb->id,0L);
	    if (!bNotBanked)
		/* always free code memory block */
		GlobalFree(pb->hMem);
	    if (ipb >= 0)
		/* pop stack and keep playing */
		pb = &pbStack[ipb];
	}
    }
}

VOID APIENTRY Wait(BOOL b)
/* install/remove NULL Playback which blocks system queue */
/* b is TRUE to install, FALSE to remove */
{

#ifdef NTPORT
OutputDebugString("START: Wait\n\r");
#endif

    if (bWait && !b) {
	/* stop waiting */
	if (ipb < 0)
	    /* no real playbacks */
#ifdef NTPORT
	    UnhookWindowsHookEx(hhkPlay);
#else
	    UnhookWindowsHook(WH_JOURNALPLAYBACK,(FARPROC) JPHook);
#endif
	bWait = FALSE;
	DelayTill = 0;
    }
    else if (!bWait && b) {
	/* start waiting */
	bWait = TRUE;
	if (ipb < 0) {
	    /* no playback - install hook */
#ifdef NTPORT
	    lpOldJPHook = SetWindowsHookEx(GetModuleHandle("recordll"),0,WH_JOURNALPLAYBACK,JPHook);
	    hhkPlay	= lpOldJPHook;
	    }
#else
	    lpOldJPHook = SetWindowsHook(WH_JOURNALPLAYBACK,(FARPROC)JPHook);
#endif
	DelayTill = GetTickCount() + MAXWAIT;
    }
}

static BOOL NEAR Nested(BYTE id)
/* see if macro with this id is already in playback */
{
    register INT i;

    for (i = 0; i <= ipb; i++)
	if (pbStack[i].id == id)
	    return TRUE;
    return FALSE;
}

/****************************************
    Hooks
****************************************/

FARPROC APIENTRY KHook(INT nCode,WPARAM wParam,LONG lParam)
/* keyboard hook.  Note that keys from JPHook do come thru here */
{
    register struct hotkey *ph;
    register INT i;
static BYTE prevkey=0;

    if (nCode == HC_ACTION) {
	if (cHotKeys > 0 && !bNoHotKeys && !bSuspend &&
	  !(ipb >= 0 && (pb->flags & B_NESTS) == 0)
	  && !(bRecording && !(RecFlags & B_NESTS))) {
	    if (prevkey == LOBYTE(wParam))
		/* previous key state bit is wrong if previous
		    call was HC_NOREM for same keystroke */
		lParam &= ~0x40000000;
	    if ((HIWORD(lParam) & 0xc000) == 0) {
		/* is keydown event.  There are hotkeys.  Hot keys enabled.
		   Not suspended in ctrl/break routine.
		   Either no playback, or one which allows nesting.
		   Either not recording, or recorded macro allows nesting */
		for (i = cHotKeys, ph = phBase; i > 0; i--, ph++) {
		    if (ph->key == LOBYTE(wParam)) {
			/* key code is right - check shifts */
			if (CheckShift(ph->flags,B_SHIFT) &&
			  CheckShift(ph->flags,B_CONTROL) &&
			  CheckShift(ph->flags,B_ALT) && !Nested(ph->id)) {
			    PostMessage(hWndRecorder,WM_REC_HOTKEY,0,
			      MAKELONG(ph->key,ph->flags));
			    prevkey = '\0';
			    /* install JP Hook to enforce delay until
				PostMessage takes */
			    Wait(TRUE);
			    /* eat key */
			    return (FARPROC) 1;
			}
		    }
		} /* end FOR all hot keys */
	    } /* end KEYDOWN */
	} /* end OKtoLookForHotKeys */
	prevkey = '\0';
    }
    else if (nCode == HC_NOREM)
	/* remember key code - prev key state may be wrong on HC_ACTION */
	prevkey = LOBYTE(wParam);

    if (bEatBreak) {
	/* ctrl/break has been detected with GetAsyncKeyState.  We should
	   eat all keys until we see it actually come through */
	if ((LOBYTE(wParam) == VK_CANCEL && HIWORD(lParam) & 0x8000)
	  || GetTickCount() > EatTimeout)
	    /* KEYUP CANCEL - we are done, else we never did see
	    CANCEL, which can happen if another app has key hook and
	    ate it */
	    bEatBreak = FALSE;
	return (FARPROC) 1;
    }

    return((FARPROC) DefHookProc(nCode,wParam,lParam,
	(LPFARPROC) &lpOldKHook));
}

static BOOL NEAR CheckShift(INT flags,INT mask)
/* return TRUE if current shift state matches that required by flags */
{
    INT state;

    state = GetKeyState(mask == B_SHIFT ? VK_SHIFT :
      (mask == B_ALT ? VK_MENU : VK_CONTROL));
    /* return TRUE if shift req and present OR not required and not
	there */
    return ((flags & mask) ? (state & 0x8000) : !(state & 0x8000));
}

FARPROC APIENTRY JPHook(INT nCode,WPARAM wParam,LPEVENTMSGMSG lpMsg)
{

    RECT rect;
    LPRECORDERMSG lptCurrent;
    LPSTR lpsz;
    HWND hWndCapture;	    /* parent popup of from hWnd from GetCapture */
    HWND hWndTarget;
    DWORD time;
    BOOL bMouseMsg,bDesktop,b;
    POINT pt;

#ifdef NTPORT
OutputDebugString("START: JPHook\n\r");
#endif

    if (nCode == HC_GETNEXT) {
	if (bDupMsg) {
	    /* playback duplicate message */
	    lmemcpy(lpMsg,&emDup,sizeof(EVENTMSG));
	    if (lpMsg->message == WM_MOUSEMOVE && ++cDups > MAXDUP) {
		/* some idiot is polling PeekMessage(NOREM).  This really
		   happens in MSDOS.EXE, who goes into such a loop on
		   DBLCLK, waiting for a WM_BUTTONUP.  The real system queue
		   does demote WM_MOUSEMOVE backwards in the queue when
		   a buttonup occurs.  MSDOS WILL NOT remove
		   an intervening WM_MOUSEMOVE.  We correct for this by
		   giving up - fill the message here and fake out the
		   HC_SKIP code into doing its thing.  Will return 0L.

		   Note that we HAVE to restrict this behavior to WM_MOUSEMOVE,
		   since I have seen MAXDUP of 10 exceeded for an ordinary
		   WM_KEYUP message.  Skipping on events that are not normally
		   "demoted" creates guaranteed playback problems. */
		nCode = HC_SKIP;
		/* remember we did this, in case get REAL skip on next call */
		ForcedSkip = 1;
	    }
	    else
		return (FARPROC) 0;
	}
	else
	    /* no longer care if we had to force a skip on previous call,
		he's reading the new msg */
	    ForcedSkip = 0;
    }

    if ((nCode == HC_SYSMODALOFF || nCode == HC_SYSMODALON) && pb->cMsgs > 1) {
	/* playback was interrupted by sysmodal dlg box, but report
	   error only if there is more to play back */

#ifdef NTPORT
	 OutputDebugString("JPHook:JPE1\n\r");
	 JPError(E_JPSYSMODAL,Dif(pb->lptS,pb->lptCurrent));
	 }
#else
	 JPError(E_JPSYSMODAL,LOWORD((DWORD)(pb->lptCurrent)));
#endif

    else if (nCode == HC_SKIP) {
	if (bWait || ForcedSkip == 2)
	    /* waiting for Recorder to do its thing - or this is a real
	       HC_SKIP immediately after a forced one (don't lose msg) */
	    return (FARPROC) 0L;
	if (ForcedSkip)
	    /* this is a forced skip */
	    ForcedSkip = 2;

	bDupMsg = bFudgeMouse = FALSE;
	hWndActivate =(HWND)-1;

	if (bAbortMode) {
	    /* aborting on ctrl/break or playback error
		NOTE: this stuff MUST be tested in same bit order as
		playback stuffing actually occurs */
	    if (ButtonFlags & BUT_LEFT)
		ButtonFlags &= ~BUT_LEFT;
	    else if (ButtonFlags & BUT_MIDDLE)
		ButtonFlags &= ~BUT_MIDDLE;
	    else if (ButtonFlags & BUT_RIGHT)
		ButtonFlags &= ~BUT_RIGHT;
	    else if (ButtonFlags & K_ALT)
		ButtonFlags &= ~K_ALT;
	    else if (ButtonFlags & K_CONTROL)
		ButtonFlags &= ~K_CONTROL;
	    else if (ButtonFlags & K_SHIFT)
		ButtonFlags &= ~K_SHIFT;

	    if (ButtonFlags == 0) {
		/* all buttons lifted - get out of here */
		bAbortMode = FALSE;
		StopPlayback(jpe.msg != 0,FALSE);
		if (jpe.msg)
		    /* (normal termination has a 0 msg field) */
		    PostMessage(hWndRecorder,jpe.msg,jpe.wParam,jpe.lParam);
	    }
	}
	else if (Break()) {
	    if (ButtonFlags) {
		bAbortMode = TRUE;
		jpe.msg = WM_JPABORT;
		jpe.wParam = 0;
		jpe.lParam = 0L;
	    }
	    else {
		StopPlayback(TRUE,FALSE);
		PostMessage(hWndRecorder,WM_JPABORT,0,0L);
	    }
	}
	else {
	    DelayTill = 0;
	    /* the SetCursorPos function is implemented by stuffing a
	       WM_MOUSEMOVE into the system queue, which we are blocking.
	       Of course, the stuffed WM_MOUSEMOVE does not show up in
	       the JR hook.  Programs like PAINT are sensitive to not
	       getting the WM_MOUSEMOVE following a SetCursorPos call, so
	       we must synthesize one, though we have no idea whether the
	       app has called SetCursorPos or not.

	       So we must check to see that the current mouse position
	       as FAR as windows is concerned agrees with our last
	       playback.  It would make a great deal of sense to call
	       GetCursorPos here and compare it against where we think
	       the mouse should be.  BUT NO!!!!  GetCursorPos will
	       NOT reflect a mouse message we have just barely played
	       back.  We must wait for the next HC_GETNEXT call before
	       things have stabilized */
	    if (bFudged) {
		/* just finished stuffing a WM_MOUSEMOVE - leave ptrs
		    alone */
		bFudged = FALSE;
		return (FARPROC) 0L;
	    }
	    /* HC_GETNEXT will decide if forced WM_MOUSEMOVE is
		necessary.  Advance ptrs anyway */
	    bFudgeMouse = TRUE;
	    bFudged = FALSE;
	    if (pb->cMsgs <= 1) {
		/* done with this playback */
		if (ButtonFlags) {
		    /* never ever ever finish with buttons down */
		    bAbortMode = TRUE;
		    jpe.msg = 0;
		    jpe.wParam = 0;
		    jpe.lParam = 0L;
		}
		else
		    StopPlayback(FALSE,FALSE);
	    }
	    else {
		/* prepare for next message */
		pb->cMsgs--;
		pb->lptCurrent++;
		time = (LONG) pb->lptCurrent->time * 55L;
		if (pb->flags & B_RECSPEED) {
		    /* set DelayTill in msec, if we haven't already
		       chewed through as much time as it took
		       during recording */
		    if (PrevMsgTime + time > GetTickCount())
			DelayTill = PrevMsgTime + time;
		}
		else {
		    /* add up time increments since last down msg
		       for bogus dbl click prevention */
		    TimeSince.lLeft += time;
		    TimeSince.lMiddle += time;
		    TimeSince.lRight += time;
		}
	    }
	}
	return (FARPROC) 0L;
    }
    else if (nCode == HC_GETNEXT) {
	/* WARNING: Windows WILL call HC_GETNEXT multiple times for
	    the same message.  You CANNOT alter state variables as
	    a result of anything you do here.  Do it in HC_SKIP */

	if ((time = GetTickCount()) < DelayTill) {
	    /* not yet */
	    if (bWait)
		/* just delay 1 tick, Wait should be canceled soon */
		return (FARPROC) 1L;
	    if (Break())
		/* user wants to break - but it is dangerous to unhook here
		   Set delay to 0, play back this msg, and pick up Break
		   again on HC_SKIP */
		DelayTill = 0;
	    else
		return (FARPROC) max((DelayTill - time)/55L,1);
	}

	if (bWait) {
	    /* Bad news - have timed out on wait
		case 1: hot key trap - WM_REC_HOTKEY to Recorder is either
		  lost or will be processed later
		case 2: request to find/activate app was lost or will
		  be done later
		No good way of handling this.  If we post an error msg,
		it will get there AFTER Recorder does the action we were
		waiting for, and may be handled severely out of context.
		Best to just get out of wait mode, resume playback if
		appropriate, and trust normal playback error checking
		to trap out if things get really wacko. */
	    Wait(FALSE);
	    if (ipb < 0)
		/* JP Hook has been removed - PB nothing, won't call again */
		return (FARPROC) 1L;
	}
	lptCurrent = pb->lptCurrent;

	/* if we blindly time-stamp each message with the current
	   GetTickCount at playback, it is possible for a slow app to take
	   so long processing the first click that the double-click
	   event doesn't get out inside the dbl-click interval, so the app
	   sees two separate clicks.  We prevent this
	   by time stamping the current event with the (previous message
	   output time + record-time interval) if this is BEFORE the
	   current GetTickCount().  The event just looks as if it happened
	   a little in the past, which is exactly what would happen for a
	   double-click in real life.  Note that PrevMsgTime is updated
	   only for REAL events output, not our bogus WM_MOUSEMOVE's, which
	   would mess us up otherwise */
	lpMsg->time = lmin(PrevMsgTime + lptCurrent->time * 55L,time);

	if (bFudgeMouse) {
	    /* see if we need to stuff a WM_MOUSEMOVE message */
	    GetCursorPos(&pt);
	    /* use bMouseMsg as random BOOL.  I hate needing zillions
	       of variables */
	    bMouseMsg = (pt.x != MouseLoc.x || pt.y != MouseLoc.y);
	    if (!bFudged) {
		/* first HC_GETNEXT call */
		if (bMouseMsg) {
		    /* cursor is not right - force WM_MOUSEMOVE with 
		       GetCursorPos location, probably set by app call
		       to SetCursorPos */
		    lmemcpy(&MouseLoc,&pt,sizeof(POINT));
		    bFudged = TRUE;
		}
		else
		    /* don't need WM_MOUSEMOVE after all - just do msg */
		    bFudgeMouse = FALSE;
	    }
	    if (bFudged) {
		lpMsg->message = WM_MOUSEMOVE;
		if (bMouseMsg)
		    SetCursorPos(lpMsg->paramL = MouseLoc.x,
		      lpMsg->paramH = MouseLoc.y);
		return PBMessage(lpMsg,FALSE);
	    }
	}

	if (bAbortMode) {
	    /* terminating playback - must lift buttons + shift keys before
	       unhooking */
	    GetCursorPos(&pt);
	    lpMsg->paramL = pt.x;
	    lpMsg->paramH = pt.y;
	    if (ButtonFlags & BUT_LEFT)
		lpMsg->message = WM_LBUTTONUP;
	    else if (ButtonFlags & BUT_MIDDLE)
		lpMsg->message = WM_MBUTTONUP;
	    else if (ButtonFlags & BUT_RIGHT)
		lpMsg->message = WM_RBUTTONUP;
	    else {
		/* lift ALT first, for easier WM_KEYUP/SYSKEYUP choice */
		if (ButtonFlags & K_ALT) {
		    lpMsg->message = WM_SYSKEYUP;
		    lpMsg->paramL = MAKEWORD(VK_MENU,scanAlt);
		}
		else {
		    lpMsg->message = WM_KEYUP;
		    if (ButtonFlags & K_CONTROL)
			lpMsg->paramL = MAKEWORD(VK_CONTROL,scanControl);
		    else if (ButtonFlags & K_SHIFT)
			lpMsg->paramL = MAKEWORD(VK_SHIFT,scanShift);
		}
		lpMsg->paramH = 1;
	    }

	    return PBMessage(lpMsg,FALSE);
	}

	lpMsg->paramL = lptCurrent->paramL;
	lpMsg->paramH = lptCurrent->paramH;

	/* keep track of which buttons are down so we can lift them
	    if necessary.  Also set BOOL for mouse message or not */
	switch(lpMsg->message =
	  (lptCurrent->message & ~(B_FORCEACTIVE|B_RESTORE))) {
	case WM_SYSKEYUP:
	case WM_KEYUP:
	    /* keyup's can go ANYWHERE, especially after system menu or
	       program close operations.  No checking is necessary */
	    switch(LOBYTE(lpMsg->paramL)) {
	    /* but we do have to remember to lift certain trouble keys */
	    case VK_MENU:
		ButtonFlags &= ~K_ALT;
		break;
	    case VK_CONTROL:
		ButtonFlags &= ~K_CONTROL;
		break;
	    case VK_SHIFT:
		ButtonFlags &= ~K_SHIFT;
	    }
	    return PBMessage(lpMsg,TRUE);
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	    bMouseMsg = FALSE;
	    break;
	default:
	    bMouseMsg = TRUE;
	}

	/* NOTE: the mouse capture has not been set if we are playing
	   back full speed into a new window which has not painted
	   yet, even though the actual event will go to the right
	   window if we just play it back anyway.  For example, after
	   playing a BUTTONDOWN into a dlg box pushbutton, it really
	   has the capture, but we won't detect it when trying
	   to play the BUTTONUP.  This is one reason why we always
	   do everything relative to the parent POPUP window, since
	   we will catch the child's capture in slow mode, and the POPUP
	   will be active in fast mode, so we're covered */

#ifdef NTPORT
//	  DebugBreak();
//	  if (bMouseMsg && (hWndCapture = GetPopParent(GetForegroundWindow())))

	if (bMouseMsg && (hWndCapture = GetPopParent(GetCapture())))
	    hWndTarget = hWndCapture;
	else
	    hWndTarget = GetForegroundWindow();
#else
	if (bMouseMsg && (hWndCapture = GetPopParent(GetCapture())))
	    hWndTarget = hWndCapture;
	else
	    hWndTarget = GetActiveWindow();
#endif

	bDesktop = FALSE;
	if (pb->flags & B_PLAYTOWINDOW) {
	    /* make sure we are playing back to the right window */

#ifdef NTPORT
	    lpsz =(LPSTR)(lptCurrent->u.szWindow+((DWORD)pb->lptS));
#else
	    lpsz =(LPSTR) MAKELONG(lptCurrent->u.szWindow,HIWORD(lptCurrent));
#endif

#ifdef WIN2
	    if (lstrcmp(lstrrchr(lpsz,'!') + 1,(LPSTR) szDesktop) == 0) {
#else
	    if (*lpsz == DESKTOPMODULE) {
#endif
		/* target is desktop, which doesn't enumerate and can't
		    be activated */
		if (!bMouseMsg) {
		    /* keys to desktop? what is this jive?? */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE2\n\r");
		    return (FARPROC) JPError(E_BADMACRO,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_BADMACRO,LOWORD((DWORD)(lptCurrent)));
#endif
		/* desktop is screen, so coords match.  Is mouse
		   over the desktop? */
		if (hWndCapture) {
		    /* desktop sure as hell doesn't have the capture */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE3\n\r");
		    return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_WRONGTARGET,LOWORD((DWORD)(lptCurrent)));
#endif
		pt.x = lptCurrent->paramL;
		pt.y = lptCurrent->paramH;
#ifdef WIN2
		if (hWndDesktop == 0) {
		    /* have not found hWndDesktop yet */
		    GetClassName(hWndTarget = WindowFromPoint(pt),
		      szScratch,CCHSCRATCH - 1);
		    if (strcmp(szDesktop,szScratch) != 0) {
			/* mouse not over desktop */
			if (!IsBenignMove(lpMsg)) {
#ifdef NTPORT
			OutputDebugString("JPHook:JPE4\n\r");
			return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
			}
#else
			return (FARPROC) JPError(E_WRONGTARGET,LOWORD(lptCurrent));
#endif
			/* else is just a harmless move anyway */
		    }
		    else
			/* remember desktop hWnd for a faster future */
			hWndDesktop = hWndTarget;
		}
#endif
		if ((hWndTarget = WindowFromPoint(pt)) != hWndDesktop
		  && !IsBenignMove(lpMsg)) {
		    /* bitch about everything except mouse move to
			desktop with no button down.  That is probably
			just a forced mouse cursor update */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE5\n\r");
		    return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_WRONGTARGET,LOWORD((DWORD)(lptCurrent)));
#endif
		bDesktop = TRUE;
	    }

	    if (!bDesktop) {
		/* NOTE: it is real tempting to try accelerating things
		   by remembering the target of the previous message and
		   skipping all the work if this msg is to the same target.
		   But there are problems, such as finding the target
		   inactive window on a sequence of WM_MOUSEMOVE's recorded
		   in EVERYTHING mode. */
		if (hWndTarget)
		    GetModClass(hWndTarget);
		if (hWndTarget == 0 || lstrcmp((LPSTR) szScratch,lpsz)) {
		    /* wrong window is active (or none at all) */
		    switch(lpMsg->message) {
		    case WM_LBUTTONUP:
		    case WM_MBUTTONUP:
		    case WM_RBUTTONUP:
		    case WM_MOUSEMOVE:
			if (hWndCapture != 0) {
			    /* if somebody has capture,
				this is messed up */
#ifdef NTPORT
			    OutputDebugString("JPHook:JPE6\n\r");
			    return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
			    }
#else
			    return (FARPROC) JPError(E_WRONGTARGET,LOWORD((DWORD)(lptCurrent)));
#endif
			/* else is mouse event to inactive window.  It must
			   exist, but don't activate it */
		    case WM_LBUTTONDOWN:
		    case WM_MBUTTONDOWN:
		    case WM_RBUTTONDOWN:
		    case WM_KEYDOWN:
		    case WM_SYSKEYDOWN:
			/* these msgs can cause an activation */
			b = FALSE;
			if (hWndActivate ==(HWND)-1)
			    /* did not try to find target hWnd yet */
			    b = TRUE;
			else if (hWndActivate) {
			    /* did find target */
			    if (!IsWindow(hWndActivate))
				/* but it was destroyed, try again */
				b = TRUE;
			    else if ((GetWindowLong(hWndActivate,GWL_STYLE)
			      & WS_CHILD)
			      && GetParent(hWndActivate) != hWndDesktop)
				/* wrong window - is a child, but not */
				/* of desktop (e. g. a combo's listbox) */
				b = TRUE;
			}
			if (b) {
			    /* all that junk above determined that we */
			    /* need to find target hWnd */
			    Wait(TRUE);
#ifdef NTPORT
			    PostMessage( hWndRecorder,
					 WM_JPACTIVATE,
					(WPARAM)pb->hMem,
					 MAKELONG(Dif(pb->lptS,lptCurrent),
						  ForceActiveFlag(lptCurrent->message)));
#else
			    PostMessage(hWndRecorder,WM_JPACTIVATE,
			      ForceActiveFlag(lptCurrent->message),
			      MAKELONG(LOWORD(lptCurrent),pb->hMem));
#endif

			    hWndActivate = 0;
			    return (FARPROC) 1L;
			}
			if (hWndActivate == 0) {
			    /* second pass - didn't find window
			      OR we did, but it was destroyed */
#ifdef NTPORT
			    OutputDebugString("JPHook:JPE7\n\r");
			    return (FARPROC) JPError(E_NOPBWIN,Dif(pb->lptS,lptCurrent));
			    }
#else
			    return (FARPROC) JPError(E_NOPBWIN,LOWORD((DWORD)(lptCurrent)));
#endif
			/* have good hWndActivate.  If msg was key down,
			   Recorder has already activated it, else
			   mouse msg will take care of itself */

#ifdef NTPORT
			if (ForceActiveFlag(lpMsg->message) > 0 &&
			  GetForegroundWindow() != hWndActivate) {
			    /* autoactivation did not take */

			    OutputDebugString("JPHook:JPE8\n\r");
			    return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
			    }
#else
			if (ForceActiveFlag(lpMsg->message) > 0 &&
			  GetActiveWindow() != hWndActivate)
			    /* autoactivation did not take */

			    return (FARPROC) JPError(E_WRONGTARGET,LOWORD((DWORD)(lptCurrent)));
#endif
			hWndTarget = hWndActivate;
			if ((pb->flags & B_WINRELATIVE) == 0 &&
			  bMouseMsg && !IsBenignMove(lpMsg)) {
			    /* window exists, but mouse will not
			       be adjusted to it below.  Check that
			       location is right */
			    GetWindowRect(hWndTarget,&rect);
			    if (!InBounds(lpMsg,&rect)){
				/* click won't go to right window */
#ifdef NTPORT
				OutputDebugString("JPHook:JPE9\n\r");
				return (FARPROC) JPError(E_WRONGTARGET,Dif(pb->lptS,lptCurrent));
				}
#else
				return (FARPROC) JPError(E_WRONGTARGET,LOWORD((DWORD)(lptCurrent)));
#endif
			    if (CheckOverlap(lpMsg,hWndTarget)) {
				/* unexpected window on top
				   of our destination - mouse will miss */
				if (!IsMouseUp(lpMsg->message)) {
				   /* Don't worry about it for button up,
				     since this is rarely real important.
				     Program manager in particular is pretty
				     loose about whether last UP on a dbl
				     click goes to him or to icon being
				     exec'ed. */
#ifdef NTPORT
				OutputDebugString("JPHook:JPE10\n\r");
				return (FARPROC) JPError(E_WINONTOP,Dif(pb->lptS,lptCurrent));
				}
#else
				return (FARPROC) JPError(E_WINONTOP,LOWORD((DWORD)(lptCurrent)));
#endif
			    }
			}
			break;
		    default:
			/* bogus message */
#ifdef NTPORT
			OutputDebugString("JPHook:JPE11\n\r");
			return (FARPROC) JPError(E_BADMACRO,Dif(pb->lptS,lptCurrent));
#else
			return (FARPROC) JPError(E_BADMACRO,LOWORD((DWORD)(lptCurrent)));
#endif
		    } /* end SWITCH(msg) on wrong active window */
		} /* end lstrcmp */
	    }
	}   /* end if (PLAYTOWINDOW) */

	if (bMouseMsg) {
	    if (!bDesktop && (pb->flags & B_WINRELATIVE)) {
		/* playback mouse coords relative to target window */
		if (hWndTarget) {
		    GetWindowRect(hWndTarget,&rect);
		    lpMsg->paramL += rect.left;
		    lpMsg->paramH += rect.top;


#ifdef NTPORT
		    if ((SHORT)lpMsg->paramL < 0	 ||
			       lpMsg->paramL >= cxScreen ||
			(SHORT)lpMsg->paramH < 0	 ||
			       lpMsg->paramH >= cyScreen)

			if (!IsBenignMove(lpMsg)) {
			    OutputDebugString("JPHook:JPE12\n\r");
			    return (FARPROC) JPError(E_OUTOFBOUNDS,Dif(pb->lptS,lptCurrent));
			    }

#else
		    if (lpMsg->paramL < 0	  ||
			lpMsg->paramL >= cxScreen ||
			lpMsg->paramH < 0	  ||
			lpMsg->paramH >= cyScreen)

			if (!IsBenignMove(lpMsg))
			    return (FARPROC) JPError(E_OUTOFBOUNDS,LOWORD((DWORD)(lptCurrent)));

#endif

		}
		if (hWndTarget == 0 || (!InBounds(lpMsg,&rect) &&
		  !IsBenignMove(lpMsg) && hWndCapture != hWndTarget)) {
		    /* out of bounds without capture - PB is messed up */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE13\n\r");
		    return (FARPROC) JPError(E_OUTOFBOUNDS,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_OUTOFBOUNDS,LOWORD((DWORD)(lptCurrent)));
#endif
	    }

#ifdef NTPORT
	    if ((pb->flags & B_PLAYTOWINDOW) &&
	      hWndTarget != GetForegroundWindow() && hWndTarget != hWndCapture) {
#else
	    if ((pb->flags & B_PLAYTOWINDOW) &&
	      hWndTarget != GetActiveWindow() && hWndTarget != hWndCapture) {
#endif
		/* check that this msg is going to the window
		    we think it is.  Main problem is another
		    window overlapping target.  But WindowFromPoint
		    does not know about newly created windows which have
		    not painted yet.  This happens when playing back into
		    a dialog box caused by a previous playback message.
		    To get around that bug, we tested for the active
		    window above.  GetCapture has same problem in
		    not knowing about new unpainted windows, but active
		    window test above should obviate the problem, unless
		    the capture is held by someone other than the active
		    window, or child thereof.  Don't worry about button
		    UP messages, since these go to the wrong app under
		    ordinary circumstances.  Also ignore case of MOVE with
		    no buttons down - probably mouse cursor update. */
		if (!IsMouseUp(lpMsg->message) &&
		  CheckOverlap(lpMsg,hWndTarget)
		  && !IsBenignMove(lpMsg)){
		    /* window under mouse doesn't belong to target app.
		       SPECIAL NOTE: it is tempting to just bring the target
		       window to top by activating it manually, but that
		       would confuse apps (like Excel) where location of
		       down click does NOT set the cursor on an activation.
		       Our activation in advance would convert mere
		       activation click into positioning click. */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE14\n\r");
		    return (FARPROC) JPError(E_WINONTOP,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_WINONTOP,LOWORD((DWORD)(lptCurrent)));
#endif
	    }

	    if (lpMsg->message != WM_MOUSEMOVE) {
		GetCursorPos(&pt);
		if (pt.x != (INT) lpMsg->paramL
		  || pt.y != (INT) lpMsg->paramH) {
		    /* some apps, including Windows move mode, are sensitive
		       to where mouse REALLY is independent of where first
		       mouse event says it is, so force issue with a
		       WM_MOUSEMOVE.  For unknown reasons, Windows sometimes
		       refuses to put the cursor where it is told, shifting
		       it a bit or two.  Since this would cause an infinite
		       loop here, check that the SetCursorPos actually took.
		       If not, forget the fudged move and output the
		       message */
		    SetCursorPos(lpMsg->paramL,lpMsg->paramH);
		    GetCursorPos(&pt);
		    if (pt.x == (INT) lpMsg->paramL
		      && pt.y == (INT) lpMsg->paramH) {
			bFudgeMouse = bFudged = TRUE;
			lpMsg->message = WM_MOUSEMOVE;
			return PBMessage(lpMsg,FALSE);
		    }
		}
	    }

	    if (hWndTarget && !IsWindowVisible(hWndTarget)
	      && !IsIconic(hWndTarget) && !IsBenignMove(lpMsg)) {
		/* this window hasn't painted yet - mouseahead doesn't
		    work properly, so we should delay until window
		    comes up.  Loop for up to MAXWAIT JPHook calls before
		    giving up.  We can't use Wait(), since that will
		    impose a long delay even though window almost always
		    paints very quickly.  It is not clear whether
		    a timer-based timeout would be preferable, since we
		    are interested in tracking the state of Windows
		    message processing, not in how long it takes to
		    do it.  A floppy-based system (yuck) would take forever
		    yet process few JPHook calls */
		if (++WaitPaint > MAXWAITPAINT) {
		    /* guess it never will paint */
#ifdef NTPORT
		    OutputDebugString("JPHook:JPE15\n\r");
		    return (FARPROC) JPError(E_PBWININVISIBLE,Dif(pb->lptS,lptCurrent));
		    }
#else
		    return (FARPROC) JPError(E_PBWININVISIBLE,LOWORD((DWORD)(lptCurrent)));
#endif
		return (FARPROC) 1L;
	    }

	    if (!(pb->flags & B_RECSPEED)) {
		/* a full speed playback can create double clicks which
		    were just two distinct clicks at record time.  Enforce
		    a delay if we are outputing two DOWN messages within
		    dbl click interval, faster than user did at record time */

		switch (lpMsg->message) {
		case WM_LBUTTONDOWN:
		    if ((time - LastDown.lLeft)
		      < (DWORD) DblClickTime &&
		      TimeSince.lLeft > (DWORD) DblClickTime)
			/* delay to ensure no bogus dbl click */
			return (FARPROC) 1L;
		    LastDown.lLeft = time;
		    TimeSince.lLeft = 0L;
		    break;
		case WM_MBUTTONDOWN:
		    if ((time - LastDown.lMiddle)
		      < (DWORD) DblClickTime &&
		      TimeSince.lMiddle > (DWORD) DblClickTime)
			/* delay to ensure no bogus dbl click */
			return (FARPROC) 1L;
		    LastDown.lMiddle = time;
		    TimeSince.lMiddle = 0L;
		    break;
		case WM_RBUTTONDOWN:
		    if ((time - LastDown.lRight)
		      < (DWORD) DblClickTime &&
		      TimeSince.lRight > (DWORD) DblClickTime)
			/* delay to ensure no bogus dbl click */
			return (FARPROC) 1L;
		    LastDown.lRight = time;
		    TimeSince.lRight = 0L;
		}
	    }

	    /* WARNING!!!  ButtonFlags must be updated ONLY after we
		are sure msg is really going to be output */
	    switch(lpMsg->message) {
	    case WM_LBUTTONDOWN:
		ButtonFlags |= BUT_LEFT;
		break;
	    case WM_MBUTTONDOWN:
		ButtonFlags |= BUT_MIDDLE;
		break;
	    case WM_RBUTTONDOWN:
		ButtonFlags |= BUT_RIGHT;
		break;
	    case WM_LBUTTONUP:
		ButtonFlags &= ~BUT_LEFT;
		break;
	    case WM_MBUTTONUP:
		ButtonFlags &= ~BUT_MIDDLE;
		break;
	    case WM_RBUTTONUP:
		ButtonFlags &= ~BUT_RIGHT;
	    }

	    MouseLoc.x = lpMsg->paramL;
	    MouseLoc.y = lpMsg->paramH;
	    GetCursorPos(&pt);
	    if (pt.x != (INT) lpMsg->paramL || pt.y != (INT) lpMsg->paramH)
		/* force cursor location to actually reflect where we think
		   it should be.  Yes, this is necessary */
		SetCursorPos(MouseLoc.x,MouseLoc.y);
	} /* end if bMouseMsg */
	else {
	    /* key - remember playback state of shift keys
	      ALWAYS a key DOWN here */
	    switch(LOBYTE(lpMsg->paramL)) {
	    case VK_MENU:
		/* remember scan code */
		scanAlt = (CHAR) HIBYTE(lpMsg->paramL);
		ButtonFlags |= K_ALT;
		break;
	    case VK_CONTROL:
		/* remember scan code */
		scanControl = (CHAR) HIBYTE(lpMsg->paramL);
		ButtonFlags |= K_CONTROL;
		break;
	    case VK_SHIFT:
		/* remember scan code */
		scanShift = (CHAR) HIBYTE(lpMsg->paramL);
		ButtonFlags |= K_SHIFT;
	    }
	}

	/* return msg and 0 delay in ticks */
	return PBMessage(lpMsg,TRUE);
    }

#ifdef NTPORT
OutputDebugString("END: JPHook\n\r");
#endif

    return((FARPROC) DefHookProc(nCode,wParam,(LONG) lpMsg,
      (LPFARPROC) &lpOldJPHook));
}

static INT NEAR ForceActiveFlag(WORD message)
{

#ifdef NTPORT
OutputDebugString("START: ForceActiveFlag\n\r");
#endif

    if (message & B_RESTORE)
	/* restore app (always activates) */
	return 2;

    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN ||
      (message & B_FORCEACTIVE))
	/* activate app */
	return 1;

    /* just find the app */
    return 0;
}

FARPROC NEAR PBMessage(LPEVENTMSGMSG lpMsg,BOOL bNew)
/* ALWAYS call this to actually play back a message */
/* bNew is TRUE for new PB message, FALSE for our own junk */
{

#ifdef NTPORT
OutputDebugString("START: PBMessage\n\r");
#endif

    if (bNew)
	/* this is a new REAL message - update PrevMsgTime */
	PrevMsgTime = lpMsg->time;
    else
	/* this is one of our own bogus messages.  Pretend that it
	   happened at the same time as our last real one */
	lpMsg->time = PrevMsgTime;

    lmemcpy(&emDup,lpMsg,sizeof(EVENTMSG));
    bDupMsg = TRUE;
    cDups = WaitPaint = 0;

    return (FARPROC) 0L;
}

static BOOL NEAR CheckOverlap(LPEVENTMSGMSG lpMsg,HWND hWndTarget)
/* return TRUE if another (non-child) window is on top of mouse coord */
{
    POINT pt;
    HWND hWnd;

#ifdef NTPORT
OutputDebugString("START: CheckOverlap\n\r");
#endif

    pt.x = lpMsg->paramL;
    pt.y = lpMsg->paramH;
    hWnd = WindowFromPoint(pt);
    return (hWnd != hWndTarget && !IsChild(hWndTarget,hWnd));
}

LONG NEAR JPError(WORD wParam,WORD offset)
/* common code to post pb error msg, stop pb, and return 1 */
/* wParam is error code */
/* offset is that of bad instruction */
{
#ifdef NTPORT
OutputDebugString("START: JPError\n\r");
//DebugBreak();
#endif

    /* disable hot keys until Recorder takes notice */
    EnaHotKeys(FALSE);
    if (ButtonFlags) {
	/* don't abort without lifting mouse buttons first */
	bAbortMode = TRUE;
	jpe.msg = WM_PBLOST;
	jpe.wParam = wParam;
	jpe.lParam = MAKELONG(offset,pb->id);
    }
    else {
	StopPlayback(TRUE,FALSE);
	PostMessage(hWndRecorder,WM_PBLOST,wParam,MAKELONG(offset,pb->id));
    }
    return 1L;
}

FARPROC APIENTRY JRHook(INT nCode,WPARAM wParam,LPEVENTMSGMSG lpMsg)
/* note that events from JP Hook do NOT go thru here, which is
   exactly what we want */
{
    if (nCode == HC_SYSMODALON || nCode == HC_SYSMODALOFF) {
	/* cannot record thru a sysmodal dialog box, but at least
	    let user record everything up to it, if desired */
	PostMessage(hWndRecorder,WM_JRBREAK,E_JRSYSMODAL,0L);
	/* just remove hook, keeping hMemTrace and state intact
	    until Recorder calls ResumeRecord or StopRecord */
#ifdef NTPORT
	UnhookWindowsHookEx(hhkRecord);
#else
	UnhookWindowsHook(WH_JOURNALRECORD,(FARPROC) JRHook);
#endif
	bSuspend = TRUE;
    }
    else if (nCode == HC_ACTION) {
	if (Break()) {
	    PostMessage(hWndRecorder,WM_JRBREAK,0,0L);
	    /* just remove hook, keeping hMemTrace and state intact
		until Recorder calls ResumeRecord or StopRecord */
#ifdef NTPORT
	    UnhookWindowsHookEx(hhkRecord);
#else
	    UnhookWindowsHook(WH_JOURNALRECORD,(FARPROC)JRHook);
#endif
	    bSuspend = TRUE;
	}

#ifdef NTPORT
	else if (GetForegroundWindow() != hWndRecorder) {
#else
	else if (GetActiveWindow() != hWndRecorder) {
#endif
	    switch(lpMsg->message) {
	    case WM_LBUTTONDOWN:
	    case WM_MBUTTONDOWN:
	    case WM_RBUTTONDOWN:
		if (MouseMode == RM_NOTHING)
		    goto skipit;
		cButtonsDown++;
		break;
	    case WM_LBUTTONUP:
	    case WM_MBUTTONUP:
	    case WM_RBUTTONUP:
		if (MouseMode == RM_NOTHING)
		    goto skipit;
		if (cButtonsDown > 0)
		    cButtonsDown--;
		break;
	    case WM_MOUSEMOVE:
		if (MouseMode == RM_EVERYTHING || 
		  (MouseMode == RM_DRAGS && cButtonsDown > 0))
		    /* either a drag, or need everything */
		    break;
		goto skipit;
	    case WM_SYSKEYDOWN:
	    case WM_KEYDOWN:
		switch(LOBYTE(lpMsg->paramL)) {
		case VK_SHIFT:
		case VK_MENU:
		case VK_CONTROL:
		    if (sysmsg.paramL == lpMsg->paramL
		      && sysmsg.message == lpMsg->message)
			/* discard rollover for shift keys */
			goto skipit;
		}
	    }
	    /* normalize time relative to previous msg */
	    if ((DWORD) lpMsg->time >= (DWORD) StartTime) {
		sysmsg.time = (StartTime == 0L) ? 0L : lpMsg->time - StartTime;
		StartTime = lpMsg->time;
	    }
	    else
		/* it is occasionally possible to get a msg time stamp
		    OLDER than the previous message, usually a WM_MOUSEMOVE
		    which has been delayed due to keystrokes coming
		    through at higher priority.  A negative delta here
		    is a LONG timeout, so just set it to 0 */
		sysmsg.time = 0L;

	    /* copy msg to our DS, also need to remember for rollover
	       handling */
	    sysmsg.paramH = lpMsg->paramH;
	    sysmsg.paramL = lpMsg->paramL;
	    sysmsg.message = lpMsg->message;
	    RecordEvent(&sysmsg);
	}
    }
skipit:
    return((FARPROC) DefHookProc(nCode,wParam,(LONG) lpMsg,
      (LPFARPROC) &lpOldJRHook));
}

static VOID NEAR RecordEvent(PEVENTMSGMSG pMsg)
/* record this event.  Called from SendMessage from JRHook */
{
    RECT rect;
    WORD x,y;
    POINT pt;
    HANDLE hMem;
    HWND hWnd,hWndP;
    LPRECORDERMSG lptCurrent;

    if (bStartUp)
	/* don't record anything until we get the first DOWN message.
	    We especially want to filter out bogus button or key UP
	    msg left over from Macro/Record dlg box, as well as irrelevant
	    mouse moves before anything real happens.  This is true
	    for resume after ctrl/break suspension too. */
	switch(pMsg->message) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	    bStartUp = FALSE;
	    break;
	default:
	    return;
	}

    x = pMsg->paramL;
    y = pMsg->paramH;

    switch(pMsg->message) {
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
	/* figure out which activatable window the mouse is going to.  Is
	    NOT current active window, since a down click on a new
	    app has not activated it yet */

#ifdef NTPORT
	if ((hWnd = GetForegroundWindow()) == 0) {
     //   if ((hWnd = GetCapture()) == 0) {
	    /* nobody has capture */
	    pt.x = x;
	    pt.y = y;

	    if (hWnd = GetForegroundWindow()) {
#else
	if ((hWnd = GetCapture()) == 0) {
	    /* nobody has capture */
	    pt.x = x;
	    pt.y = y;

	    if (hWnd = GetActiveWindow()) {
#endif
		/* active window may not be displayed yet, so it overrides
		    WindowFromPoint if point is in bounds */
		GetWindowRect(hWnd,&rect);
		if ((INT) x >= (INT) rect.left && (INT) x <= (INT) rect.right
		  && (INT) y >= (INT) rect.top
		  && (INT) y <= (INT) rect.bottom) {
		    /* point is inside bounds of active window */

		    /* the following looks for a special case where another
		       window is on top of active window, and is the actual
		       target.  Primo example is the popup listbox of
		       a combobox.  These puppies are WS_CHILD windows
		       with GetParent() desktop and GetWinWord(PARENT)
		       the active window */
		    if ((hWndP = WindowFromPoint(pt)) && hWndP != hWnd) {
			/* window under point is NOT the active window */
			if (!(GetWindowLong(hWndP,GWL_STYLE) & WS_CHILD)
			  || GetParent(hWndP) == hWndDesktop) {
			    /* ... and is toplevel, OR child of desktop */
			    if (IsChild(hWnd,hWndP)
			      || IsOwnedBy(hWnd,hWndP)) {
				/* .. and is descendent of active app */
				/* Make THIS the target */
				hWnd = hWndP;
			    }
			}
		    }
		    goto gotit;
		}
	    }
#ifdef NTPORT
	    if ((hWnd = WindowFromPoint(pt)) == 0)
		/* no window under point - use active window */
		hWnd = GetForegroundWindow();
#else
	    if ((hWnd = WindowFromPoint(pt)) == 0)
		/* no window under point - use active window */
		hWnd = GetActiveWindow();
#endif
	}
gotit:
	/* don't actually want capture or window under the point, since it
	    is likely to be a child.  We really want the first
	    parent which can be active, since that is where
	    the mouse should be relative to, and is a lot easier to
	    deal with at playback time */
	hWnd = GetPopParent(hWnd);

	if (RecFlags & B_WINRELATIVE) {
	    /* adjust coords for all mouse msgs */
	    GetWindowRect(hWnd,&rect);
	    x = pMsg->paramL - rect.left;
	    y = pMsg->paramH - rect.top;
	    break;
	}
	break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
	if (cMsgs == 1)
	    /* despite filter above, can still get unwanted extra key ups
		at start of recording (extra shift, ctrl, etc) */
	    return;
    case WM_KEYDOWN:

#ifdef NTPORT
    case WM_SYSKEYDOWN:
	hWnd = GetForegroundWindow();
    }

    if (GETHWNDINSTANCE(hWnd) == GETHWNDINSTANCE(hWndRecorder))
	/* never record anything for Recorder */
	return;
#else
    case WM_SYSKEYDOWN:
	hWnd = GetActiveWindow();
    }

    if (GetWindowWord(hWnd,GWW_HINSTANCE) ==
      GetWindowWord(hWndRecorder,GWW_HINSTANCE))
	/* never record anything for Recorder */
	return;
#endif

    if ((cMsgs % CEVENTALLOC) == 0) {
	/* need more space */

	hMem = GlobalReAlloc(hMemTrace,
	  (LONG) (cMsgs + CEVENTALLOC) * sizeof(RECORDERMSG),
	  GMEM_MOVEABLE|GMEM_NOT_BANKED|GMEM_SHARE);

	if (hMem == 0) {
	    JRError(E_NOMEM);
	    return;
	}
	else
	    hMemTrace = hMem;
    }

    GetModClass(hWnd);

    lptCurrent = ((LPRECORDERMSG) GlobalLock(hMemTrace)) + cMsgs;
    if ((lptCurrent->u.ps = AddString(szScratch)) == 0) {
	GlobalUnlock(hMemTrace);
	JRError(E_NOMEM);
	return;
    }
    lptCurrent->message = pMsg->message;

    /* time hooked in msec, want in ticks */
    lptCurrent->time = (WORD) (pMsg->time/55L);
    if (pMsg->time % 55 > 27)
	lptCurrent->time++;

    lptCurrent->paramL = x;
    lptCurrent->paramH = y;
    GlobalUnlock(hMemTrace);

    cMsgs++;
    if (LOWORD((DWORD)(lptCurrent)) > 50000) {
	/* need lots of room for window names too */
	PostMessage(hWndRecorder,WM_JRBREAK,E_TOOLONG,0L);
	/* remove hook, keeping hMemTrace and state intact
	    until Recorder calls StopRecord */
#ifdef NTPORT
	UnhookWindowsHookEx(hhkRecord);
#else
	UnhookWindowsHook(WH_JOURNALRECORD,(FARPROC) JRHook);
#endif
	bSuspend = TRUE;
    }
}

static BOOL NEAR IsOwnedBy(HWND hWndOwner,HWND hWnd)
/* return TRUE if hWnd is owned by hWndOwner */
{
    HWND hWndS;

#ifdef NTPORT
OutputDebugString("START: IsOwnedBy\n\r");
#endif


    hWndS = hWnd;
    while (hWnd) {
	if (hWnd == hWndOwner)
	    return TRUE;
#ifdef NTPORT
	hWnd = GETHWNDPARENT(hWnd);
#else
	hWnd = (HWND)GetWindowWord(hWnd,GWW_HWNDPARENT);
#endif

    }

    hWnd = hWndS;
    while (hWnd) {
	if (hWnd == hWndOwner)
	    return TRUE;
	hWnd = GetParent(hWnd);
    }

    hWnd = hWndS;

#ifdef NTPORT
    if (GETHWNDPARENT(hWnd) == NULL && GetParent(hWnd) == hWndDesktop) {
#else
    if (GetWindowWord(hWnd,GWW_HWNDPARENT) == NULL
      && GetParent(hWnd) == hWndDesktop) {
#endif
	/* NULL parent and desktop owner - c'mon now, SOMEBODY owns this */
	/* sucker!!!  (WinWord's combobox listboxes do this) */

#ifdef NTPORT
	if (GETHWNDINSTANCE(hWnd) == GETHWNDINSTANCE(hWndOwner))
	    return TRUE;
#else
	if (GetWindowWord(hWnd,GWW_HINSTANCE) == GetWindowWord(hWndOwner,GWW_HINSTANCE))
	    return TRUE;
#endif

    }
    return FALSE;
}

VOID NEAR JRError(WORD ecode)
/* common code to handle journal record error */
{
    /* no hot keys until error msg box comes down */
    EnaHotKeys(FALSE);
    /* stop recording (but save if just a TOOLONG error) */
    StopRecord((struct macro FAR *) NULL, ecode);
    PostMessage(hWndRecorder,WM_JRABORT,ecode,0L);
}

static HWND NEAR GetPopParent(HWND hWnd)
/* return activatable parent of hWnd */
{
    HWND hWnd2;

    if (!IsWindow(hWnd))
	/* probably a NULL */
	return hWnd;
    do {
	if ((GetWindowLong(hWnd,GWL_STYLE) & WS_POPUP)
	  == WS_POPUP)
	    /* is a popup, ok */
	    return hWnd;
	if (hWnd2 = GetParent(hWnd)) {
	    if (hWndDesktop == 0) {
		/* have not found hWndDesktop yet */
#ifdef WIN2
		GetClassName(hWnd2,szScratch,CCHSCRATCH - 1);
		if (strcmp(szDesktop,szScratch) == 0)
		    /* got it */
		    hWndDesktop = hWnd;
#else
		hWndDesktop = GetDesktopWindow();
#endif
	    }
	    if (hWnd2 == hWndDesktop)
		/* this window's parent is desktop - stop right here */
		return hWnd;
	    hWnd = hWnd2;
	}
	else
	    /* got to top parent */
	    return hWnd;
    } while (1);
}

static BOOL NEAR Break()
/* return TRUE if user wants to break */
{
#ifdef REMOTECTL
    if (bBreakPB) {
	EnaHotKeys(bBreakPB = FALSE);
	return TRUE;
    }
#endif
    if (bEatBreak)
	/* break already detected, but not cleared */
	return TRUE;
    if (bBreakCheck && (GetAsyncKeyState(VK_CONTROL) & 0x8001)
      && (GetAsyncKeyState(VK_CANCEL) & 0x8001)) {
	/* turn off new hot keys until Recorder gets WM_JPABORT */
	bEatBreak = TRUE;
	EatTimeout = GetTickCount() + MAXEATTIME;
	/* disable hotkeys until Recorder wakes up */
	EnaHotKeys(FALSE);
	return TRUE;
    }
    return FALSE;
}

static BOOL NEAR InBounds(LPEVENTMSGMSG lpMsg,LPRECT lpRect)
{
    /* have to do integer comparisons, since edges may well be in negative
	territory off the screen */
    return ((INT) lpMsg->paramL >= (INT) lpRect->left
      && (INT) lpMsg->paramL <= (INT) lpRect->right
      && (INT) lpMsg->paramH >= (INT) lpRect->top
      && (INT) lpMsg->paramH <= (INT) lpRect->bottom);
}

static VOID NEAR GetModClass(HWND hWnd)
/* put MODULE!CLASS of hWnd in szScratch */
{
    register CHAR *pch;
    INT cch;
    HANDLE hInst;
    HANDLE hmem;

#ifndef WIN2
    if (hWnd == GetDesktopWindow()) {
	szScratch[0] = DESKTOPMODULE;
	szScratch[13] = '\0';	       // Note: I changed this from from
	return; 		       // 1 to 13 for the index -- JOHNSP
    }
#endif

#ifdef NTPORT
    hInst = GETHWNDINSTANCE(hWnd);
#else
    hInst = (HANDLE)GetWindowWord(hWnd,GWW_HINSTANCE);
#endif

    szScratch[GetModuleFileName(hInst,szScratch,CCHSCRATCH - 1)] = '\0';

    /* strip down to just root of module name */
    pch = szScratch + (cch = strlen(szScratch));
    while (cch-- > 0) {
	switch(*pch) {
	case '.':
	    /* get rid of extension */
	    *pch = '\0';
	    break;
	case '\\':
	case '/':
	case ':':
	    /* found end of path or drive name, move root portion
		to start of scScratch */
	    strcpy(szScratch,pch + 1);
	    /* and get out of here */
	    cch = 0;
	}
	pch--;
    }
    pch = szScratch + (cch = strlen(szScratch));
    *pch++ = '!';

#ifdef NTPORT
   // OutputDebugString("JustBefore GetClassName() in Hook\n\r");
   // DebugBreak();
    GetClassName(hWnd,pch,CCHSCRATCH - cch - 2);
   // strcpy(pch,"Clock");  // just to test further until fix in sys
#else
    EnumWindows(REnumProc,(LONG)hWnd);
    strcpy(pch,lpszCls);
#endif

    szScratch[max(CCHSCRATCH - 1,strlen(szScratch))] = '\0';

}

BOOL APIENTRY REnumProc( HWND hwnd, LONG l) {

    OutputDebugString("START:REnumProc\n\r");
  //  DebugBreak();

    if ( hwnd == (HWND)l ) {
	GetClassName(hwnd,lpszCls,100);
	return FALSE;
	}

    return TRUE;

}

VOID NEAR FreeStringList()
{
    register struct stringlist *ps;

    while (ps = psFirst) {
	psFirst = psFirst->psNext;

#ifdef NTPORT
	GlobalFree((HANDLE) ps);
#else
	LocalFree((HANDLE) ps);
#endif
    }
    psFirst = 0;
}

static struct stringlist *AddString(CHAR *sz)
{
    register struct stringlist *ps;
    INT rc;
    struct stringlist *psNew,*psPrev;

    psPrev = 0;
    for (ps = psFirst; ps; psPrev = ps, ps = ps->psNext) {
	if ((rc = strcmp(ps->sz,sz)) == 0)
	    return ps;
	if (rc > 0)
	    break;
    }
#ifdef NTPORT
    /* insert new one here */
    if ((psNew = (struct stringlist *) GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT|GMEM_SHARE,
      sizeof(struct stringlist) + strlen(sz))) == 0)
	return 0;
#else
    /* insert new one here */
    if ((psNew = (struct stringlist *) LocalAlloc(LPTR,
      sizeof(struct stringlist) + strlen(sz))) == 0)
	return 0;
#endif

    strcpy(psNew->sz,sz);
    if (psPrev == 0) {
	/* empty list or new string > first */
	psNew->psNext = psFirst;
	psFirst = psNew;
    }
    else {
	psNew->psNext = psPrev->psNext;
	psPrev->psNext = psNew;
    }

    return psNew;
}

static LPSTR lstrrchr(LPSTR lpsz,UCHAR c)
{
    while (*lpsz) {
	if ((UCHAR) *lpsz == c)
	    return lpsz;
#ifdef DBCS
	lpsz = AnsiNext(lpsz);			/* DBCS */
#else
	lpsz++;
#endif
    }
    return (LPSTR) 0L;
}

static LONG NEAR lmin(LONG l1,LONG l2)
{
    return l1 < l2 ? l1 : l2;
}

static BOOL NEAR IsMouseUp(WORD m)
{
    return (m == WM_LBUTTONUP || m == WM_MBUTTONUP || m == WM_RBUTTONUP);
}

static BOOL NEAR IsBenignMove(LPEVENTMSGMSG lpMsg)
/* return TRUE if msg is MouseMove, but KeyState table says no mouse button
   is down */
{
    if (lpMsg->message != WM_MOUSEMOVE)
	return FALSE;

    return !(GetKeyState(VK_LBUTTON) < 0 || GetKeyState(VK_RBUTTON)
      || GetKeyState(VK_MBUTTON));
}

#ifdef NTPORT
WORD Dif( LPRECORDERMSG lptS, LPRECORDERMSG lptC)
{
LONG lDif;

    lDif =(LONG)((LONG)lptS-(LONG)lptC);
    if (lDif < 0) lDif = lDif*-1;

    return LOWORD(lDif);

}

HWND GetForegroundWindow( VOID ) {

#ifdef NTPORT
OutputDebugString("START: GetForegroundWindow\n\r");
#endif


    return GetWindow(GetDesktopWindow(),GW_CHILD);
}

#endif

/* ------------------------------ EOF --------------------------------- */
