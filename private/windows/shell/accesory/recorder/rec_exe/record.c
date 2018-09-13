/**************************************************************************
NAME

    record.c -- macro record functions

SYNOPSIS



DESCRIPTION


RETURNS


CAUTIONS


AUTHOR

    Steve Squires

HISTORY

    By		Date		Description
    --------    ----------      -----------------------------------------
    sds		6 Dec 88	Initial Coding
    sds		9 Dec		Added flash timer
				Now only auto-activate previous window
				  on PB if not PLAYTOWINDOW
				Uses AddMacro to append new macro
				JPActivate now goes for class name match to
				  distinguish popups.
    sds		14 Dec		Upgrade to global szMSDOS, etc.
    sds		15 Dec		added caps, num, scrl lock support
    sds		19 Dec		cMsgs now in paramL of 1st msg
				now using PMACRO
				now keeping macro in EMM, copy to unbanked
				  for pb only
    sds		30 Dec		added CloseTDyn call
    sds		3 Jan 89	now uses global szWinOldApp
				set Tracer active on record break
		5 Jan		record break now uses better dlg box
				fixed bug in pbID hack
		6 Jan		removed unused RecordAbort
				static'ed some internals
		9 Jan		no longer activate on just a module
				  name match
		10 Jan		Efficiency improvements
		12 Jan		Hotkey now only autoactivates other app
				  if Tracer is active
				Improved macro copy from banked to unbanked
				  memory
		13 Jan		Improved dlgInt handling
		1 Feb 89	Now setting font face and size at record time
		1 Mar 89	removed font support
		14 Mar 89	now using ForceTrActive
		16 Mar 89	now clearing wTimerID (bug)
		30 Mar 1989	new LdStr with hWnd
		4 Apr 1989	bug fix - wasn't always clearing pmNew
		5 Apr 1989	now using RecordBreak for ALL recording
		6 Apr 1989	record suspend/Resume changed
		26 Apr 1989	JPActivate now can restore too
		2 May 1989	DoDlg now handles dlgbox failure (NOMEM)
		13 May 1989	now releases macro code segment more often
		16 May 1989	takes advantage of protectmode to not
				use GMEM_NOT_BANKED memory
		19 May 1989	PlayHotKey now returns success
		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	bProtMode now bNotBanked, bNoFlash
    sds		7 Aug 1989	bug fix for ctrl/numlock == pause
				now doing PeekABoo and retry on JPActivate
				failure
    sds		10 Aug 1989	JPActivate now loops and PeekABoo's for up
				to as long as record-time msg interval
    sds		17 Aug 1989	playback id now set in AddMacro, passed
				to AddHotKey
    sds		18 Sep 1989	RecordStop now sets bSuspended
    sds		20 Sep 1989	now smarter about iconizing on playback,
				especially if in menu mode
    sds		31 Oct 1989	FreeMacro now takes extra arg
    sds		28 Nov 1989	no longer clicks on desktop to bring menus
				down before playback.  Win30 bug fixed.
    sds		8 Jan 1990	force window active on playback now checks
				for disabled window.  No longer restores
				a zoomed window.
    sds		19 Jan 1990	added extra check that hWndPrevActive is
				not a child, in case hWnd was reassigned
    sds 	26 Oct 1990	now waits at LEAST MAXWAIT for window
				checks for user break during this period
    sds 	7 Jan 1991	cleanups for MSC6

*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */

#include <windows.h>
#include <port1632.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */
static BOOL NEAR CheckBreak(VOID);
INT rstrcmp( LPSTR, LPSTR );

/* VARIABLES DEFINED */
static HWND hWndActivate=0;

/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */

VOID RecordStart()
/* pmNew already initialized */
{
    HANDLE hMem;

    if ((hMem = GlobalAlloc(
      bNotBanked ? (GMEM_MOVEABLE|GMEM_SHARE) : (GMEM_MOVEABLE|GMEM_NOT_BANKED|GMEM_SHARE),
      (LONG) CEVENTALLOC * sizeof(RECORDERMSG))) == 0) {
	ErrMsg(hWndRecorder,E_NOMEM);
	return;
    }

    /* record caps, scroll, numlock key states at record time */
    if (GetKeyState(VK_CAPITAL) & 1)
	pmNew->f.flags |= B_CAPSLOCK;
    if (GetKeyState(VK_NUMLOCK) & 1)
	pmNew->f.flags |= B_NUMLOCK;
    if (GetKeyState(VK_SCROLLLOCK) & 1)
	pmNew->f.flags |= B_SCRLOCK;

    /* first msg contains header stuff
	do not assign hMemCode - tdyn may realloc to new hMem anyway */
    ((LPRECORDERMSG) GlobalLock(hMem))->message = pmNew->f.flags;
    GlobalUnlock(hMem);
    StartRecord(hMem,MouseMode,pmNew->f.flags);

    wTimerID = SetTimer( (HWND)NULL,
			  0,
			  FLASHTIMER,
			 (FARPROC)lpfnWndProc);
    bRecording = TRUE;
}

VOID RecordStop(BOOL bKeepIt)
{
    INT e;
    HANDLE hMem;

    if (!bRecording)
	return;

    if (wTimerID) {
	KillTimer(NULL,wTimerID);
	wTimerID = 0;
    }
    /* make sure icon is back to normal */
    FlashWindow(hWndRecorder,FALSE);
    e = StopRecord((struct macro FAR *)pmNew, bKeepIt);

    bRecording = FALSE;
#ifdef REMOTECTL
    bNoFlash = FALSE;
#endif
    if (!bKeepIt)
	/* just chuck it */
	FreeMacro(pmNew,FALSE);
    else {
	if (e) {
	    /* LOWORD contained error msg */
	    PostMessage(hWndRecorder,WM_POSTERROR,e,0L);
	    FreeMacro(pmNew,FALSE);
	}
	else {
	    if (!bNotBanked) {
		/* move code segment out of scarce nonbanked memory */
		hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,(LONG) pmNew->f.cchCode);
		if (hMem == 0) {
		    PostMessage(hWndRecorder,WM_POSTERROR,E_NOMEM,0L);
		    FreeMacro(pmNew,FALSE);
		}
		else {
		    lmemcpy(GlobalLock(hMem), GlobalLock(pmNew->hMemCode),
		      pmNew->f.cchCode);
		    GlobalUnlock(hMem);
		    GlobalUnlock(pmNew->hMemCode);
		    GlobalFree(pmNew->hMemCode);
		    pmNew->hMemCode = hMem;
		}
	    }
	    else
		hMem =(HANDLE) 1;

	    if (hMem) {
		/* add to macro list (before adding hot key) */
		AddMacro(pmNew);
		if (pmNew->f.hotkey)
		    if (!AddHotKey(pmNew->f.hotkey,pmNew->f.flags,pmNew->id))
			PostMessage(hWndRecorder,WM_POSTERROR,E_TOOMANYHOT,0L);
		LoadMacroList();
		/* select newest macro */
		SendMessage(hWndList,LB_SETCURSEL,
		  (WORD)SendMessage(hWndList,LB_GETCOUNT,0,0L),0L);
		bFileChanged = TRUE;
	    }
	}
    }
    pmNew = 0;
}

VOID RecordBreak()
/* called on ctrl/break or Recorder activation */
{
    INT rc;

    if (!bRecording)
	return;

    SuspendRecord(bSuspended = TRUE);
    ForceRecActivate(TRUE);
    LdStr(hWndRecorder,M_JRBREAK);
    rc = DoDlg(hWndRecorder,(FARPROC)dlgInterrupt,DLGINT);
    /* need correct window active for stuffed CONTROL KEYUP */
    ForceRecActivate(FALSE);
    switch(rc) {
    case IDRESUME: /* continue recording */
	SuspendRecord(FALSE);
	break;
    case IDSAVE:  /* recording is done */
	RecordStop(TRUE);
	break;
    case IDTRASH:	/* forget the whole thing */
    case 0:		/* couldn't run dlg box - out of memory?? */
	RecordStop(FALSE);
    }
    bSuspended = FALSE;
}

BOOL APIENTRY EnumProc(HWND hWnd,LONG lParam)
/* loword(lParam) is szWindow - find it and set hWndActivate */
{
    register CHAR *pch;
    INT cch;
    HANDLE hInst;

    /* the window to be activated may be a true parent or a popup.
       If we get a match through the module and class name, we
       definitely have it */

    /* get the module name and class in szScratch */

#ifdef NTPORT
    OutputDebugString("START:EnumProc\n\r");
    hInst =GETHWNDINSTANCE(hWnd);
#else
    hInst =(HANDLE)GetWindowWord(hWnd,GWW_HINSTANCE);
#endif

	szScratch[GetModuleFileName(hInst,szScratch,80)] = '\0';
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
    GetClassName(hWnd,pch,CCHSCRATCH - cch - 2);
    szScratch[max(CCHSCRATCH - 1,strlen(szScratch))] = '\0';

#ifdef NTPORT
    if (rstrcmp(szScratch,(CHAR *)lParam) == 0) {
#else
    if (strcmp(szScratch,(CHAR *) LOWORD(lParam)) == 0) {
#endif
	hWndActivate = hWnd;
	return FALSE;
    }
    return TRUE;
}

#ifdef NTPORT
VOID JPActivate(LONG lng, HANDLE hMem)
{
    LPRECORDERMSG lpt,lptS;
    CHAR szTemp[CCHSCRATCH];
    DWORD timeout;
    BOOL  flag;
    LPSTR lpstr;

    flag = (BOOL)HIWORD(lng);
    lptS = (LPRECORDERMSG) GlobalLock(hMem);
    lpt  = (LPRECORDERMSG)((DWORD)lptS+LOWORD(lng));

    lpstr=(LPSTR)((DWORD)lptS+(UINT)(lpt->u.szWindow));
    lstrcpy((LPSTR)szTemp,lpstr);

    if ((timeout = (DWORD)lpt->time * 55L) > (DWORD) MAXWAIT)
	/* wait at least MAXWAIT */
	timeout = (DWORD) MAXWAIT;

#else
VOID JPActivate(LONG lng, INT flag)
/* JP Hook needs to activate an app.  Find it and call SetPBWindow */
/* lng LOWORD is offset of event, HIWORD is hMem */
/* flag 0 to find, 1 to activate, 2 to activate and restore */
{
    LPRECORDERMSG lpt;
    CHAR szTemp[CCHSCRATCH];
    HANDLE hMem;
    DWORD timeout;

    hMem = HIWORD(lng);
    lpt = (LPRECORDERMSG) ((LONG) GlobalLock(hMem) | (LONG) LOWORD(lng));

    /* need window name in DS */

    lstrcpy((LPSTR) szTemp,(LPSTR)MAKELONG(lpt->u.szWindow,HIWORD(lpt)));
    if ((timeout = (DWORD)lpt->time * 55L) > (DWORD) MAXWAIT)
	/* wait at least MAXWAIT */
	timeout = (DWORD) MAXWAIT;
#endif

    timeout += GetTickCount() - 55L;
    GlobalUnlock(hMem);

    hWndActivate = 0;

    /* when playing back to most apps, the target window is always there
	before the next JPHook call.  For example, there is usually no
	Peek/GetMessage	between a menu pick and the resulting dialog box
	coming up.  But an interpretive language target app will call
	PeekMessage a LOT to be polite while executing uninteresting
	internal code.  JPHook WILL get called well before there is any
	hint of bringing up the dialog box, etc. that we are looking for
	here.  The only apparent and reasonable solution is to hang out
	here and keep looking for as long as it took at record time.
	This is limited	somewhat by MAXWAIT */
    do {

#ifdef NTPORT
	EnumWindows(lpfnEnum,(LONG)(&szTemp[0]));
#else
	EnumWindows(lpfnEnum,MAKELONG(szTemp,0));
#endif
	if (hWndActivate)
	    /* got it */
	    break;
	if (CheckBreak())
	    /* user abort */
	    return;
	/* tickle DLL so it won't time out prematurely */
	Wait(TRUE);
	PeekABoo();
    } while (GetTickCount() < timeout);

    if (hWndActivate && flag > 0 && IsWindowEnabled(hWndActivate)) {
	/* is a kbd msg.  We must do activation manually */
#ifdef NTPORT
	SetForegroundWindow(hWndActivate);
#else
	SetActiveWindow(hWndActivate);
#endif
	if (flag == 2 && IsIconic(hWndActivate))
	    /* restore it too */
	    SendMessage(hWndActivate,WM_SYSCOMMAND,SC_RESTORE,0L);
    }

    SetPBWindow(hWndActivate);
}

BOOL PlayHotKey(BYTE key,BYTE flags)
/* playback macro on a hot key */
{
    register PMACRO pm;

    if (key == VK_PAUSE &&
      ((flags & (B_SHFT_CONT_ALT)) == B_CONTROL))
	/* DLL sees ctrl/numlock as pause, xlate it back to find hotkey */
	key = VK_NUMLOCK;
    for (pm = pmFirst; pm; pm = pm->pmNext)
	if(pm->f.hotkey == key	 &&
	  (pm->f.flags&(B_SHFT_CONT_ALT))==(WORD)(flags&(B_SHFT_CONT_ALT)))
	    {
	    PlayStart(pm,TRUE);
	    return TRUE;
	    }
    return FALSE;
}

VOID PlayStart(PMACRO pm,BOOL bOnHotKey)
{
    INT rc;
    HANDLE hMem;

    if (rc = LoadMacro(pm)) {
	PostMessage(hWndRecorder,WM_POSTERROR,rc,0L);
	return;
    }

    if ((pm->f.flags & B_PLAYTOWINDOW) == 0
      && hWndRecorder == GetActiveWindow())
	/* playback won't auto-activate anybody, so hit the one
	    active before Recorder */
	if (hWndPrevActive && IsWindow(hWndPrevActive) &&
	  !IsIconic(hWndPrevActive) &&
	  !(GetWindowLong(hWndPrevActive,GWL_STYLE) & WS_CHILD))
#ifdef NTPORT
	    SetActiveWindow(hWndPrevActive);
#else
	    SetForegroundWindow(hWndPrevActive);
#endif

    if (bMinimize && !IsIconic(hWndRecorder)) {
	/* make ourselves iconic */
#ifdef OLD
/* Windows 2.x and early versions of 3.0 had severe problems with
    menu mode.  You could not change the active window, minimize, etc.
    This has been fixed for Win3.0.  Code left here for reference in
    truly fixing the problem for Win2.x version */

	if (GetActiveWindow() == hWndRecorder && GetCapture()) {
	    /* may be in menu mode - blow us out of it */
	    PostMessage( GetDesktopHwnd(),WM_LBUTTONDOWN,0,0L);
	    /* can't do minimize until menu actually comes down, post
		msg to pick up where we left off and return */
	    PostMessage(hWndRecorder,WM_MINPLAYBACK,(WORD)pm,
	      MAKELONG(bOnHotKey,0));
	    return;
	}
#endif
	ShowWindow(hWndRecorder,SW_MINIMIZE);
    }

    if (!bNotBanked) {
	/* put macro in unbanked memory for dynalink */
	if ((hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_NOT_BANKED|GMEM_SHARE,
	  (LONG) pm->f.cchCode)) == 0) {
	    PostMessage(hWndRecorder, WM_POSTERROR, E_NOMEM, 0L);
	    return;
	}
	lmemcpy(GlobalLock(hMem), GlobalLock(pm->hMemCode), pm->f.cchCode);
	GlobalUnlock(pm->hMemCode);
	GlobalUnlock(hMem);
	/* don't need our code block anymore */
	ReleaseCode(pm);
    }
    else
	hMem = pm->hMemCode;

    if (rc = StartPlayback(hMem, pm->id)) {
	/* we free segment here on error - else it is recordll's
	   responsibility */
	if (bNotBanked)
	    ReleaseCode(pm);
	else
	    /* GMEM_NOT_BANKED */
	    GlobalFree(hMem);
	if (bOnHotKey) {
	    if (rc == E_TOONESTED) {
		/* report error only if too nested (otherwise is already
		   running) */
		PlayStop();
		PostMessage(hWndRecorder,WM_POSTERROR,rc,0L);
	    }
	}
	else
	    ErrMsg(hWndRecorder,rc);
    }
}

VOID PlayStop()
{
    StopPlayback(TRUE,FALSE);
}

VOID ShutDown()
/* called when program closing - undo hooks */
{
    StopRecord((struct macro FAR *)NULL, FALSE);
    StopPlayback(TRUE,FALSE);
    FreeMacroList();
    CloseRDLL();
}

static BOOL NEAR CheckBreak()
{
    if (bBreakCheck && (GetAsyncKeyState(VK_CONTROL) & 0x8001)
      && (GetAsyncKeyState(VK_CANCEL) & 0x8001)) {
	/* user abort */
	PostMessage(hWndRecorder,WM_JPABORT,0,0L);
	StopPlayback(TRUE,TRUE);
	return TRUE;
    }
    return FALSE;
}

INT rstrcmp( LPSTR lpsz, LPSTR lpsz2 ) {

// Temporary routine until workaround is in place.


    while((*lpsz != '!') && (*lpsz != '\0'))
	lpsz++;

    while((*lpsz2 != '!') && (*lpsz2 != '\0'))
	lpsz2++;

    OutputDebugString("rstrcmp just before strcmp\n\r");
    // DebugBreak();

    if(strcmp(lpsz,lpsz2))
	 return 1;
    else return 0;

}

/* ------------------------------ EOF --------------------------------- */
