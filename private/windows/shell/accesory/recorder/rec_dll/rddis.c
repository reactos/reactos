/************************************************************************
NAME

    rddis.c -- dynalink for recorder applet - discardable code

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
    sds		6 Dec 88	Initial Coding
		9 Dec		Sets bFirstMouse on any new record start
		14 Dec 88	Added bStartUp
		20 Dec		fixed bStartup bug
		30 Dec		now records screen size
				permanent kbd hook, added CloseTDyn
		3 Jan 89	LastDown init'ed here to be global,
				  across macros
				fixed startup bug
		4 Jan 89	Heuristics now adjusts timing for discarded
				  msgs
		5 Jan 89	Heuristics now discards trailing WM_MOUSEMOVE
				  msgs
		6 Jan		SethWnd now InitTDyn, returns VERSION
		12 Jan		bHaveMouseMsg now set in Heuristics
		6 Feb 89	Fixed bHaveMouseMsg bug in Heuristics
    sds		28 Feb 89	Fixed unbalanced lock/unlock bug
				now using UnhookWindowsHook to unhook
    sds		27 Mar 1989	now discarding trailing shift/down msgs
    sds		28 Mar 1989	now clears old BREAK's when starting record
    sds		3 Apr 1989	Heuristics no longer allows macro to end
				with naked KEYDOWN
    sds		6 Apr 1989	Changed recording suspension handling
    sds		26 Apr 1989	added B_FORCEACTIVE bit to buttondown
				msg following junked alt/tab/esc sequence
    sds		16 May 1989	taking advantage of protectmode
    sds		12 Jun 1989	bProtMode now bNotBanked
    sds		7 Aug 1989	fixed ctrl/numlock hotkey bug - is a PAUSE
    sds		10 Aug 1989	fixed bug adding time delay of trashed
				ctrl/tab or ctrl/esc messages
    sds		17 Aug 1989	AddHotKey now takes id
    sds 	8 Jan 1990	fixed bogus B_RESTORE bit setting code
    sds		27 Nov 1990	clear sysmsg.message on start record
    sds 	7 Jan 1991	cleanup for MSC6
    johnsp	13 Feb 1991
*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */
#include <windows.h>
#include <port1632.h>
#include <string.h>
#include "recordll.h"
#include "msgs.h"
#include "rdlocal.h"


/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */
static VOID NEAR CleanResidue(VOID);
static BOOL NEAR CheckMsg(LPRECORDERMSG,WORD,BYTE);

/* VARIABLES DEFINED */

#ifdef NTPORT
extern HHOOK hhkPlay;
extern HHOOK hhkRecord;
extern HHOOK hhkKey;
#endif


/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */
UINT APIENTRY InitRDLL(HWND hWnd,BOOL bIsNotBanked)
/* MUST be called at startup so recordll knows who to talk to */
{

    if(!MLocalInit(0,0,512-1)) return 0;

    hWndRecorder = hWnd;
    bNotBanked	 =(BBOOL)bIsNotBanked;
    DblClickTime =(DWORD)(GetDoubleClickTime()+55);
    cxScreen	 =(WORD)(GetSystemMetrics(SM_CXSCREEN));
    cyScreen	 =(WORD)(GetSystemMetrics(SM_CYSCREEN));

    /* we need kbd hook at all times, to catch ctrl/break during record */

#ifdef NTPORT
    if ((LONG) lpOldKHook == 0L) {
	lpOldKHook = SetWindowsHookEx(GetModuleHandle("recordll"),0,WH_KEYBOARD,KHook);
	hhkKey=lpOldKHook;
	}
#else
    if ((LONG) lpOldKHook == 0L)
	lpOldKHook = SetWindowsHook(WH_KEYBOARD,(FARPROC) KHook);
#endif

    return TVERSION;
}

VOID APIENTRY CloseRDLL()
/* MUST be called at Recorder termination */
{
    if ((LONG) lpOldKHook != 0L) {

#ifdef NTPORT
	UnhookWindowsHookEx(hhkKey);
#else
	UnhookWindowsHook(WH_KEYBOARD,(FARPROC) KHook);
#endif
	lpOldKHook = (FARPROC) 0L;
    }
}

VOID APIENTRY EnableBreak(BOOL b)
{
    bBreakCheck = (BBOOL) b;
}

VOID APIENTRY StartRecord(HANDLE h,INT mouMode,WORD recflags)
{
    if (!bSuspend) {
	/* not just continuing from a suspend */
	FreeStringList();
	hMemTrace = h;
	MouseMode = mouMode;
	RecFlags = recflags;
	bHaveMouseMsg = FALSE;
	cMsgs = 1;
    }
#ifdef NTPORT
    lpOldJRHook = SetWindowsHookEx(GetModuleHandle("recordll"),0,WH_JOURNALRECORD,JRHook);
    hhkRecord= lpOldJRHook;
#else
    lpOldJRHook = SetWindowsHook(WH_JOURNALRECORD,(FARPROC) JRHook);
#endif

    cButtonsDown = 0;
    StartTime = 0L;
    sysmsg.message = 0;
    bSuspend = FALSE;
    bRecording = bStartUp = TRUE;
    /* clear any leftover BREAK keys */
    GetAsyncKeyState(VK_CANCEL);
}

VOID APIENTRY SuspendRecord(BOOL b)
{
    if (b) {
	/* suspend recording */
	if (bRecording && !bSuspend) {
	    UnhookWindowsHook(WH_JOURNALRECORD,(FARPROC) JRHook);
	    bSuspend = TRUE;
	}
    }
    else if (bSuspend) {
	/* throw away events leading up to the suspension */
	CleanResidue();
	StartRecord(hMemTrace,MouseMode,RecFlags);
    }
}

static VOID NEAR CleanResidue()
/* at end of recording, or at breakpoint in recording.  Clear
   out any junk messages leading up to this point */
{
    LPRECORDERMSG lpM;
    lpM = ((LPRECORDERMSG) GlobalLock(hMemTrace)) + cMsgs - 1;
    while (cMsgs > 1 && (lpM->message == WM_MOUSEMOVE ||
      CheckMsg(lpM,WM_SYSKEYDOWN,VK_MENU) ||
      CheckMsg(lpM,WM_SYSKEYDOWN,VK_TAB) ||
      CheckMsg(lpM,WM_SYSKEYDOWN,VK_ESCAPE) ||
      CheckMsg(lpM,WM_SYSKEYUP,VK_ESCAPE) ||
      CheckMsg(lpM,WM_KEYDOWN,VK_CONTROL) ||
      CheckMsg(lpM,WM_KEYDOWN,VK_SHIFT))) {
	cMsgs--;
	lpM--;
    }
    GlobalUnlock(hMemTrace);
}

BOOL APIENTRY AddHotKey(BYTE key,WORD flags,BYTE id)
{
    register struct hotkey *ph;

    if (cHotKeys == 0) {
#ifdef NTPORT
	if ((phBase = (struct hotkey *)
	  GlobalAlloc(GMEM_FIXED|GMEM_ZEROINIT|GMEM_SHARE,sizeof(struct hotkey))) == 0)
	    return FALSE;
#else
	if ((phBase = (struct hotkey *)
	  LocalAlloc(LPTR,sizeof(struct hotkey))) == 0)
	    return FALSE;
#endif
    }
    else {
#ifdef NTPORT
	ph = (struct hotkey *)GlobalReAlloc((HANDLE) phBase,
	  (cHotKeys + 1) * sizeof(struct hotkey),GMEM_MOVEABLE|GMEM_SHARE);
	if (ph == 0)
	    return FALSE;
	phBase = ph;
#else
	ph = (struct hotkey *)LocalReAlloc((HANDLE) phBase,
	  (cHotKeys + 1) * sizeof(struct hotkey),LMEM_MOVEABLE);
	if (ph == 0)
	    return FALSE;
	phBase = ph;
#endif
    }
    ph = phBase + cHotKeys++;
    ph->flags = (BYTE)(LOBYTE(flags)&B_SHFT_CONT_ALT);

    if ((ph->flags & B_SHFT_CONT_ALT)==B_CONTROL &&
      key == VK_NUMLOCK)
	/* ctrl/numlock comes in as a PAUSE */
	ph->key = VK_PAUSE;
    else
	ph->key = key;
    ph->id = id;
    return TRUE;
}

BOOL APIENTRY DelHotKey(BYTE key,WORD flags)
{
    register struct hotkey *ph;
    INT i;

    if (key == 0 && flags == 0) {
	/* free everything */
	if (cHotKeys > 0) {
#ifdef NTPORT
	    GlobalFree((HANDLE) phBase);
#else
	    LocalFree((HANDLE) phBase);
#endif
	    cHotKeys = 0;
	    phBase = 0;
	}
	return TRUE;
    }

    /* only remembered relevant bits, don't get confused by others,
	which may have changed */
    flags &= B_SHFT_CONT_ALT;
    for (ph = phBase, i = cHotKeys; i > 0; i--, ph++)
	if (ph->key == key && ph->flags == LOBYTE(flags)) {
	    /* (overlapped memcpy is ok for dest < src) */
	    memcpy(ph,ph + 1,(i - 1)*sizeof(struct hotkey));
	    cHotKeys--;
	    return TRUE;
	}
    return FALSE;
}

VOID APIENTRY EnaHotKeys(BOOL b)
{
    bNoHotKeys = (BBOOL) !b;
}

INT NEAR Heuristics()
/* clean up recorded macro */
{
    LPRECORDERMSG lpM,lpM1;
    UINT time;
    HANDLE hMem;
    INT i,j,k;
    BOOL b,bRestore;

    /* always throw away trailing junk */
    CleanResidue();
    lpM = ((LPRECORDERMSG) GlobalLock(hMemTrace)) + cMsgs - 1;
    if (cMsgs > 1 && (lpM->message == WM_SYSKEYDOWN ||
      lpM->message == WM_KEYDOWN)) {
	/* last event is a keydown.  If we play this back as is, we wind
	    up restoring the keystate table before this last message
	    is processed, which can really confuse things. */
	b = TRUE;
    }
    else
	b = FALSE;

    GlobalUnlock(hMemTrace);
    if (cMsgs <= 1)
	return 0;

    if (b) {
	/* add a bogus KEYUP event */
	if ((cMsgs % CEVENTALLOC) == 0) {
	    /* need more space */
	    hMem = GlobalReAlloc( hMemTrace,
				 (LONG)(cMsgs + 1)*sizeof(RECORDERMSG),
				  GMEM_MOVEABLE|GMEM_NOT_BANKED|GMEM_SHARE);
	    if (hMem == 0)
		return E_NOMEM;
	    else
		hMemTrace = hMem;
	}
	lpM1 = ((LPRECORDERMSG) GlobalLock(hMemTrace)) + cMsgs;
	lpM = lpM1 - 1;
	/* doesn't really matter who the target app is */
	lpM1->u.ps = lpM->u.ps;
	lpM1->message = (lpM->message == WM_SYSKEYDOWN ?
	  WM_SYSKEYUP : WM_KEYUP);
	lpM1->paramL = lpM->paramL;
	lpM1->paramH = lpM->paramH;
	lpM1->time = 0;
	GlobalUnlock(hMemTrace);
	cMsgs++;
    }

    lpM = ((LPRECORDERMSG) GlobalLock(hMemTrace)) + 1;

    /* i is always the number of msgs remaining, counting current one */
    for (i = cMsgs - 1; i > 0; i--, lpM++) {
	switch(lpM->message & ~B_FORCEACTIVE) {
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
	    bHaveMouseMsg = TRUE;
	    if (MouseMode == RM_EVERYTHING)
		/* not trashing alt-tabs, just needed to know about mouse
		    msgs.  Get out of FOR loop now. */
		i = 0;
	    break;
	case WM_SYSKEYDOWN:
	    if (MouseMode != RM_EVERYTHING &&
	      CheckMsg(lpM,WM_SYSKEYDOWN, VK_MENU)) {
		/* ALT key down */
		b = FALSE;
		j = i - 1;
		lpM1 = lpM + 1;
		/* look for ALT/ESC or ALT/TAB sequence */
		while (j >= 2 &&
		  ((CheckMsg(lpM1,WM_SYSKEYDOWN, VK_ESCAPE) &&
		    CheckMsg(lpM1 + 1, WM_SYSKEYUP, VK_ESCAPE)) ||
		  ((CheckMsg(lpM1,WM_SYSKEYDOWN, VK_TAB) &&
		    CheckMsg(lpM1 + 1, WM_SYSKEYUP, VK_TAB))))) {
		    /* definitely want to trash these msgs */

		    /* TAB UP does RESTORE of an icon.  May need to do
			so on playback */
		    bRestore = CheckMsg(lpM1 + 1, WM_SYSKEYUP, VK_TAB);
		    j -= 2;
		    lpM1 += 2;
		    b = TRUE;
		}
		if (b) {
		    /* have a real alt/esc/tab sequence.  JPHook will
		       auto-activate correct app on next message anyway, so
		       discard this whole mess */
		    if (j > 0) {
			if (CheckMsg(lpM1, WM_SYSKEYUP, VK_MENU)
			  || CheckMsg(lpM1,WM_KEYUP,VK_MENU)) {
			    /* got ALT key up.  Discard DOWN and UP */
			    j--;
			    lpM1++;
			}
			else {
			    /* next msg expects alt key to be down, keep
				the initial DOWN message */
			    lpM++;
			    i--;
			}
		    }
		    if (j > 0) {
			for (k = (i - j) - 1, time = 0; k >= 0; k--)
			    /* add up timing of discarded msgs */
			    time += (lpM + k)->time;
			lmemcpy((LPSTR) lpM,(LPSTR) lpM1,
			  j * sizeof(RECORDERMSG));
			/* expand next msg delay to include skipped msgs */
			lpM->time += time;
		    }

		    cMsgs -= (i - j);
		    i = j;

		    if (j > 0)
			/* throwing away alt/tab and alt/esc loses the
			    information that an app was already active
			    before a msg, which matters.  Set
			    forceactive and restore bits if necessary */
			for (lpM1 = lpM, j = cMsgs; j > 0; lpM1++, j--) {
			    switch(lpM1->message) {
			    case WM_LBUTTONDOWN:
			    case WM_MBUTTONDOWN:
			    case WM_RBUTTONDOWN:
			    case WM_KEYDOWN:
			    case WM_SYSKEYDOWN:
				lpM1->message |= bRestore
				  ? (B_FORCEACTIVE|B_RESTORE) : B_FORCEACTIVE;
			    }
			    if (lpM1->message != WM_MOUSEMOVE)
				/* just looking for first non-move msg */
				break;
			}
		}
	    }
	}
    }

    GlobalUnlock(hMemTrace);
    return 0;
}

static BOOL NEAR CheckMsg(LPRECORDERMSG lpMsg,WORD message,BYTE key)
{
    return (lpMsg->message == message && key == LOBYTE(lpMsg->paramL));
}
/* ------------------------------ EOF --------------------------------- */
