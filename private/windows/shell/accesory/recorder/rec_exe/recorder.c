/**************************************************************************
NAME

    recorder.c -- Windows Macro Recorder

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
		9 Dec		flashes window during record
				some new create time loadstr's
		12 Dec		Fixed failure to report bad file from
				cmd line, and to init hot keys
		14 Dec		Now install record hook BEFORE going
				iconic and activating other app.
				Loading new strings
		15 Dec		now loads szWin200
		19 Dec		now using PMACRO
		20 Dec		now retaining active window on PB error
				  or abort notification
				Fixed ctrl/break PB abort handling
		30 Dec		now gets cxScreen, cyScreen
		3 Jan 89	Now loads szWinOldApp
				Better about making ourselves active
				  or abort msgs
		5 Jan		hot keys now disabled during dlg box
		6 Jan		WarnEm is now static near
		9 Jan		LoadStr now null's first byte of szScratch
		10 Jan		Hotkeys now disabled during Mod/Delete
		24 Feb 89	fixed activation stuff at record time
		14 Mar 89	Improved self-activation code for error
				reporting, etc.
		16 Mar 89	now clearing wTimerID (bug)
				stops recording on ALT-UP or LBUTTON DOWN
				  instead of ACTIVATE (for ALT-ESC'ing)
				improved error reporting
		17 Mar 89	no longer tolerates ZOOM or oversize
		25 Mar 1989	LoadResource assertion does better error
		29 Mar 1989	fixed bug in ForceTrActivat'ion
				better shutdown handling
		30 Mar 1989	LdStr now takes parent hWnd arg
		6 Apr 1989	now does GETMINMAXINFO, break dlg box on
				any form of record stop
		26 Apr 1989	changed stop-record on activate handling
		27 Apr 1989	now does break on sysmodal dlg in record mode
		2 May 1989	DoDlg now handles dlgbox failure (NOMEM)
		3 May 1989	absolutely positively refuses to restore
				window if still recording
		9 May 1989	KeyWndProc
		11 May 1989	WIN2 fix
		16 May 1989	tdyn.h now tracerd.h
		18 May 1989	added REMOTECTL stuff
		18 May 1989	massive user intfc changes
		22 May 1989	ID_CMDLINE support
		23 May 1989	now releasing code segment after playback
		25 May 1989	now does dir change on cmd from other tracer
		26 May 1989	WIN2 upgrade
		7 Jun 1989	changed macro delete handling
		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	bProtMode now bNotBanked
    sds		30 Jun 1989	fixed listbox index bug in DeleteMacro
				Merge option now permanently on.  OK
				  to merge into blank "new" file.
    sds		3 Jul 1989	now using wsprintf
    sds		27 Jul 1989	no longer using toupper
				.hlp loaded from .RC
    sds		7 Aug 1989	PeekABoo now public
    sds		9 Aug 1989	Delete of non-hotkey macro now displays
				  comment in confirmation msgbox
    sds		29 Aug 1989	HELP menu changes
    sds		18 Sep 1989	fixed bug losing recording on exit from Win
    sds		20 Sep 1989	WM_MINPLAYBACK
    sds		21 Sep 1989	better handling of pb error during recording
    sds		29 Sep 1989	resizing out gap under listbox on WM_SIZE
    sds		10 Oct 1989	listbox now NOINTEGRALHEIGHT, removed unused
				  code
				yet another change to WinHelp calls
   				cmdline from new instance now better about
				  saving existing data
    sds		16 Oct 1989	delete of last macro now retains active
				  file.  File/Save deletes it.
    sds		30 Oct 1989	fixed nested hotkey disable problem
    sds		31 Oct 1989	FreeMacro change, added WinHelp(QUIT)
				WM_POSTERROR now calls ErrMsgArg
    sds		22 Nov 1989	workaround for listbox scrolling first
				macro out of site on File/Load Recorder x.rec
    sds		30 Nov 1989	changed MB_ICONQUESTION to MB_ICONEXCLAMATION
    sds		4 Dec 1989	WIN2 compatibility fix
    sds		6 Dec 1989	accelerator handling
    sds		19 Jan 1990	yet another listbox paint problem fix
				hourglass on File/Save
				more careful checking validity of hWnd
				  to activate after Recorder popup
    sds		31 Jan 1990	fixed bug leaving icon flashed off after
				  PB abort notification
    sds		8 Feb 1990	now set focus to hWndRecorder if LB empty
    sds 	15 Feb 1990	fixed benign bug in DoCaption
    sds 	26 Oct 1990	bBreakCheck
    sds 	7 Jan 1991	fcn def changes for MSC6

*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */

#include <windows.h>
#include <port1632.h>
#if WIN32
#include <shellapi.h>
#endif
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <dos.h>

#include <direct.h>
#include <ctype.h>
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */
#ifndef WIN2
static BOOL NEAR WarnEm(VOID);
#endif
static PMACRO FindPick(INT *pi);
static VOID NEAR SearchAndDestroy(VOID);


/* VARIABLES DEFINED */
static BBOOL bSuspendMode = FALSE;   /* handling break during record mode */
static BBOOL bWaitActive = FALSE;   /* waiting in ForceTrActivate */

/* FUNCTIONS REFERENCED */


/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */
/* Procedures which make up the main window class. */
LONG APIENTRY WndProc(HWND hWnd, WORD message, WPARAM wParam, LONG lParam)
{
    HWND hWndTemp;
    LPSTR lpsz;
    PMACRO pm;
    INT rc,i;
#ifdef WIN2
static BOOL bResize=FALSE;
#endif
#ifdef REMOTECTL
    BOOL b;
#endif

    switch (message) {
    case WM_SIZE: 
	if (IsWindow(hWndList) && wParam != SIZEICONIC &&
#ifdef WIN2
	  wParam != SIZEZOOMHIDE && !bResize) {
	    RECT rect,rectP;
	    bResize = TRUE;
#else
	  wParam != SIZEZOOMHIDE) {
#endif
	    /* size listbox to fill client area.  It will round down,
	       so resize ourselves to stop at bottom of listbox
	       Repaint happens inside MoveWindow */
	    MoveWindow(hWndList,0,0,LOWORD (lParam),HIWORD (lParam), TRUE);
	    /* listbox bug workaround.  If you do File/Load Recorder FOO.REC,
	       the first line of listbox scrolls out of view.  Force
	       selection/scroll into view. */
	    SendMessage(hWndList,LB_SETCURSEL,
	      (WORD) SendMessage(hWndList,LB_GETCURSEL,0,0L),0L);
#ifdef WIN2
	    /* how high is client window? */
	    GetClientRect(hWnd,(LPRECT) &rect);
	    rc = rect.bottom - rect.top;
	    /* how big is listbox, really? */
	    GetWindowRect(hWndList,(LPRECT) &rect);
	    /* size Recorder down by the difference */
	    GetWindowRect(hWnd,(LPRECT) &rectP);
	    /* prevent repaint of listbox - not needed */
	    SetWindowLong(hWndList,GWL_STYLE,
	      GetWindowLong(hWndList,GWL_STYLE) & ~WS_VISIBLE);
	    SetWindowPos(hWnd,NULL,rectP.left,rectP.top,
	      rectP.right - rectP.left, rectP.bottom - rectP.top
	      - (rc - (rect.bottom - rect.top)),
	      SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	    SetWindowLong(hWndList,GWL_STYLE,
	      GetWindowLong(hWndList,GWL_STYLE) | WS_VISIBLE);
	    bResize = FALSE;
#endif
	}

	if (!bSuspendMode && bRecording)
	    switch(wParam) {
	    case SIZEFULLSCREEN:
	    case SIZENORMAL:
		/* make sure we stop recording on SIZEFULL or SIZENORMAL
		    This is only place we detect alt-tab to us */
		bSuspendMode = TRUE;
		RecordBreak();
		bSuspendMode = FALSE;
	    }
	break;
    case WM_SETFOCUS:
	if (pmFirst)
	    /* no focus unless have at least one macro */
	    SetFocus(hWndList);
	break;
    case WM_DESTROY:
	ShutDown();
        PostQuitMessage(0);
        break;
    case WM_INITMENU:
	EnableMenuItem(hMenuRecorder,ID_SAVEAS,
	  pmFirst ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
	EnableMenuItem(hMenuRecorder,ID_SAVE,
	  (szFileName || pmFirst) ? MF_ENABLED : MF_DISABLED|MF_GRAYED);

	/* can't edit, delete, or playback without macros */
	rc = (pmFirst ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
	EnableMenuItem(hMenuRecorder,ID_EDIT,rc);
	EnableMenuItem(hMenuRecorder,ID_DELETE,rc);
	EnableMenuItem(hMenuRecorder,ID_PLAYBACK,rc);
	break;
    case WM_QUERYOPEN:
	return !(bSuspendMode || bRecording);
    case WM_SYSKEYUP:
	if (wParam != VK_MENU || GetActiveWindow() != hWndRecorder)
	    return(DefWindowProc(hWnd, message, wParam, lParam));
	/* FALL THRU!!! */
    case WM_NCLBUTTONDOWN:
    case WM_LBUTTONDOWN:
	/* we stop recording on ALT UP and LBUTTONDOWN down messages
	    instead of plain old WM_ACTIVATE, since ALT-ESC sequences
	    give us a WM_ACTIVATE even if user is just skipping through */
	if (!bSuspendMode && bRecording) {
	    /* except when suspended */
	    PostMessage(hWnd,WM_JRBREAK,1,0L);
	}
	return(DefWindowProc(hWnd, message, wParam, lParam));

    case WM_CREATE:
	CreateTime(hWnd);
	break;

    case WM_PBLOST: /* playback got lost */
	pbe.ecode = wParam;
	pbe.id = (BYTE) HIWORD(lParam);
	pbe.offset = LOWORD(lParam);
	ForceRecActivate(TRUE);
	DoDlg(hWndRecorder,(FARPROC)dlgPBError,DLGPBERROR);
	ForceRecActivate(FALSE);
	if (bRecording)
	    /* break recording too */
	    PostMessage(hWnd,WM_JRBREAK,M_PBERECORD,0L);
	break;

    case WM_PBDONE: /* playback completed */
	if (bNotBanked) {
	    /* dynalink does not free code when NOT copying to low memory
	       - get rid of it now */
	    for (pm = pmFirst; pm; pm = pm->pmNext)
		if (pm->id == (BYTE) wParam) {
		    ReleaseCode(pm);
		    break;
		}
	}
	break;
    case WM_JPABORT:	/* playback aborted by user */
	ForceRecActivate(TRUE);
	/* use about box dlg code to recognize OK button and trash
	   ctrl/break coming out of system queue */
	bCenterDlg = TRUE;
	DoDlg(hWndRecorder,(FARPROC)dlgAbout,DLGRECORDEROK);
	ForceRecActivate(FALSE);
	if (bRecording)
	    /* break recording too */
	    PostMessage(hWnd,WM_JRBREAK,M_BREAKRECORD,0L);
	break;
    case WM_JRABORT:	/* record aborted, error msg in wParam */
	RecordStop(FALSE);
	ForceRecActivate(TRUE);
	ErrMsg(hWndRecorder,wParam);
	/* leave recorder active */
	break;
    case WM_JPACTIVATE: /* playback to new window */

#ifdef NTPORT
	JPActivate(lParam,(HANDLE)wParam);
#else
	JPActivate(lParam,(BOOL)wParam);
#endif
	break;
    case WM_JRBREAK:	/* break during record */
	fBreakType = wParam;
	bSuspendMode = TRUE;
	RecordBreak();
	fBreakType = 0;
	bSuspendMode = FALSE;
	break;
    case WM_REC_HOTKEY:
	/* OK, we got it.  OK to continue */
	Wait(FALSE);
	PlayHotKey((BYTE) LOWORD(lParam),(BYTE) HIWORD(lParam));
	break;
    case WM_MINPLAYBACK:
	/* we just posted and processed a LBUTTONDOWN to hWndDesktop to blow
	  us out of menu mode before minimizing ourselves.  This msg tells us
	  to do the minimize and play back the macro */
	ShowWindow(hWndRecorder,SW_MINIMIZE);
	PlayStart((PMACRO) wParam,(BOOL) LOWORD(lParam));
	break;
    case WM_POSTERROR:
	/* this msg is for when we are
	   in an odd state, not necessarily active, where we want to
	   let things settle a bit before reporting problem */
	ForceRecActivate(TRUE);
	ErrMsgArg(hWnd,wParam,szErrArg);
	break;
    case WM_TIMER:
	/* flash icon during record or wait for activation mode.
	   Note that hWnd is NULL due to the way we created timer */
#ifdef REMOTECTL
	if (bWaitActive || !bNoFlash)
#endif
	    FlashWindow(hWndRecorder,TRUE);
	break;
    case WM_SYSCOMMAND:
	if (bWaitActive)
	    /* we are in ForceRecActivate.  a click on our flashing
	       icon can send us into a brain-dead menu mode which never
	       returns from the DispatchMessage call, so we sit there
	       flashing while the user looks at our system menu.
	       Prevent this nonsense by blowing off this message */
	    break;
	if (bRecording || bSuspendMode)
	    /* still recording - stay iconic and let WM_JRBREAK come
		thru */
	    switch (wParam) {
	    case SC_RESTORE:
		if (!bSuspendMode)
		    /* alt/tab to us is detected here for break */
		    PostMessage(hWnd,WM_JRBREAK,1,0L);
		/* FALL THRU */
	    case SC_MAXIMIZE:
	    case SC_KEYMENU:
	    case SC_MOUSEMENU:
#ifdef WIN2
	    case 0xf012:    /* undocumented SC_MOUSEMENU equivalent */
#endif
		return 0L;
	    }
	return(DefWindowProc(hWnd, message, wParam, lParam));

#ifdef NTPORT
    case WM_COMMAND:
	switch(GET_WM_COMMAND_ID(wParam,lParam)) {
	case ID_PLAYBACK:
	    if (pm = FindPick(&i))
		PlayStart(pm,FALSE);
	    break;
	case ID_LISTBOX:
	    if (GET_WM_COMMAND_CMD(wParam,lParam)== LBN_DBLCLK)
		SendMessage(hWnd,WM_COMMAND,GET_WM_COMMAND_MPS(ID_PLAYBACK,0,0));
	    break;
#else
    case WM_COMMAND:
	switch(wParam) {
	case ID_PLAYBACK:
	    if (pm = FindPick(&i))
		PlayStart(pm,FALSE);
	    break;
	case ID_LISTBOX:
	    if (HIWORD(lParam) == LBN_DBLCLK)
		SendMessage(hWnd,WM_COMMAND,ID_PLAYBACK,0L);
	    break;
#endif
	case ID_NEW:
	    if (!WarnEm())
		return FALSE;
	    FreeFile();
	    return TRUE;
	case ID_OPEN:
	    if (!WarnEm())
		break;
	    DoDlg(hWndRecorder,(FARPROC)dlgOpen,DLGOPEN);
	    DoCaption();
	    break;
	case ID_MERGE:
	    DoDlg(hWndRecorder,(FARPROC)dlgMerge,DLGMERGE);
	    break;

#ifdef WIN32   /* NTPORT */
	/* Note: The GET_WM_COMMAND_CMD() is not used here.
		 Reason for this is to compensate for the
		 HANDLE TO MEMORY being passed in.  This unusual
		 call is being made from rinit.c */

	case ID_CMDLINE:
	    lpsz = GlobalLock((HANDLE)lParam);

	    /* first string is cwd of calling recorder */

	    for (rc = 0; rc < CCHSCRATCH - 1 && *lpsz != ' ';)
		szScratch[rc++] = *lpsz++;

	    if (*lpsz == ' ') {
		szScratch[rc] = '\0';
		SetCurrentDirectory((LPSTR)szScratch);
		DoCmdLine(++lpsz);
	    }

	    /* else is some jive, ignore it */
	    GlobalUnlock((HANDLE)lParam);
	    break;
#else
	case ID_CMDLINE:
	    lpsz = GlobalLock((HANDLE) LOWORD(lParam));
	    /* first string is cwd of calling recorder */
	    if (*(lpsz + 1) == CHDRIVESEPARATOR) {
		/* change drive */
		bdos(0xe,*lpsz - 'A',0);
		lpsz += 2;
	    }
	    for (rc = 0; rc < CCHSCRATCH - 1 && *lpsz != ' ';)
		szScratch[rc++] = *lpsz++;
	    if (*lpsz == ' ') {
		szScratch[rc] = '\0';
		_chdir(szScratch);
		DoCmdLine(++lpsz);
	    }
	    /* else is some jive, ignore it */
	    GlobalUnlock((HANDLE) LOWORD(lParam));
	    break;
#endif
	case ID_SAVE:
	    if (szFileName) {
		if (!pmFirst) {
		    /* save of empty file deletes it */
		    LdStr(hWndRecorder,M_DELETEEMPTY);
		    if ((rc = MessageBox(hWndRecorder,szScratch,szCaption,
		      MB_OKCANCEL)) == IDOK) {
			/* bye-bye */
			_unlink(ofCurrent.szPathName);
	     		FreeFile();
			return TRUE;
		    }
		    return FALSE;
		}

		szErrArg = szFileName;
		SetCursor(hCurWait);
		rc = RecWriteFile(szFileName);
		SetCursor(hCurNormal);
		if (rc) {
		    ErrMsgArg(hWndRecorder,rc,szErrArg);
		    if (hFileOut !=-1) {
			_lclose(hFileOut);
			hFileOut =-1;
		    }
		    return (LONG) FALSE;
		}
		else
		    WriteDone();
		return (LONG) TRUE;
	    }
	    /* FALL THROUGH!!! */
	case ID_SAVEAS:
	    if (pmFirst) {
		rc = DoDlg(hWndRecorder,(FARPROC)dlgSave,DLGSAVE);
		DoCaption();
	    }
	    else
		/* init time or passed cmd line */
		rc = TRUE;
	    return rc;
	case ID_EXIT:
	    PostMessage(hWnd,WM_CLOSE,0,0L);
	    break;
	case ID_RECORD:
	    if (DoDlg(hWndRecorder,(FARPROC)dlgRecord,DLGRECORD)) {
	    	/* important to install record hook BEFORE going iconic,
		    else may be slow enough for user to enter events
		    before we start recording.  Such events can get through
		    to the app despite the fact that we never pass control
		    back to Windows before installing hook.  I don't
		    understand it either */
		RecordStart();
		ShowWindow(hWndRecorder,SW_MINIMIZE);
	    }
	    break;
	case ID_DELETE:
	    SearchAndDestroy();
	    break;
	case ID_EDIT:
	    if (pmEdit = FindPick(&i)) {
		if (GetKeyState(VK_SHIFT) < 0)
		    /* temp hack to display commands */
		    DoDlg(hWnd,(FARPROC)dlgCmds,DLGCMDS);
		else {
		    if (DoDlg(hWnd,(FARPROC)dlgEdit,DLGEDIT)) {
			/* update listbox */
			SendMessage(hWndList,LB_DELETESTRING,i,0L);
			HotKeyString(pmEdit,TRUE);
			SendMessage(hWndList,LB_INSERTSTRING,i,
			  (LONG) (LPSTR) szScratch);
			/* re-select this line */
			SendMessage(hWndList,LB_SETCURSEL,i,0L);
		    }
		}
	    }
	    break;
	case ID_ABOUT:
        {   CHAR szExtraInfo[100];
	    HANDLE hInst;

#ifdef NTPORT
	    hInst=GETHWNDINSTANCE(hWnd);
#else
	    hInst=(HANDLE)GetWindowWord(hWnd, GWW_HINSTANCE);
#endif

	    LoadString(hInst, M_EXTRAINFO, szExtraInfo, 100);

	    ShellAbout(hWnd, szCaption, szExtraInfo, LoadIcon(hInst,
		       (LPSTR)"Recorder"));

	    break;
        }
	case ID_HOTKEY:
	    EnaHotKeys(bHotKeys = !bHotKeys);
	    CheckMenuItem(hMenuRecorder,ID_HOTKEY,
	      bHotKeys ? MF_CHECKED : MF_UNCHECKED);
	    break;
	case ID_BREAK:
	    EnableBreak(bBreakCheck = !bBreakCheck);
	    CheckMenuItem(hMenuRecorder,ID_BREAK,
	      (BOOL) (bBreakCheck ? MF_CHECKED : MF_UNCHECKED));
	    break;
	case ID_MINIMIZE:
	    /* get current state and invert */
	    bMinimize = !CheckMenuItem(hMenuRecorder,ID_MINIMIZE,MF_CHECKED);
	    CheckMenuItem(hMenuRecorder,ID_MINIMIZE,
	      (BOOL) (bMinimize ? MF_CHECKED : MF_UNCHECKED));
	    break;
	case ID_PROFILE:
	    DoDlg(hWndRecorder,(FARPROC)dlgPref,DLGPREF);
	    break;
#ifndef WIN2

#ifdef WINHELPFIXED
        /* Note that the standard help #'s are assigned to these
           messages in recorder.h */
        case ID_USEHELP: /* 0xFFFC */
            WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, (DWORD) 0);
	    break;
#endif
        case ID_INDEX:    /* 0xFFFF */
	case ID_COMMANDS:
	case ID_PROCEDURES:
        case ID_KEYBOARD:    /* 0x001D */
	    if (wParam == ID_INDEX) {
		i = HELP_INDEX;
		wParam = 0;
	    }
	    else
		i = HELP_CONTEXT;
	    strcat(strcpy(szScratch,szCaption),szDotHlp);
#ifdef WINHELPFIXED
	    WinHelp(hWnd, (LPSTR)szScratch, (WORD) i, (DWORD) wParam);
#endif
            break;
#endif
	}
	break;

    case WM_QUERYENDSESSION:
	return (WarnEm() ? 1L : 0L);
    case WM_CLOSE:
	if (!WarnEm())
	    break;
#ifndef WIN2
	strcat(strcpy(szScratch,szCaption),szDotHlp);

#ifdef WINHELPFIXED
	WinHelp(hWnd, (LPSTR)szScratch, (WORD) HELP_QUIT, (DWORD) 0);
#endif

#endif
	return(DefWindowProc(hWnd, message, wParam, lParam));

#ifdef NTPORT
    case WM_ACTIVATE:
	/* we remember the app that was active before us in order to set it
	    active when we start to record, or resume recording */
	if (GET_WM_ACTIVATE_STATE(wParam,lParam)) {
	    if (IsWindow(hWndTemp = (HWND)GET_WM_ACTIVATE_HWND(wParam,lParam)))
		if (GetParent(hWndTemp) != hWndRecorder)
		    /* not one of our dlg boxes */
		    hWndPrevActive = hWndTemp;
	}
#else
    case WM_ACTIVATE:
	/* we remember the app that was active before us in order to set it
	    active when we start to record, or resume recording */
	if (wParam) {
	    if (IsWindow(hWndTemp = (HWND) LOWORD(lParam)))
		if (GetParent(hWndTemp) != hWndRecorder)
		    /* not one of our dlg boxes */
		    hWndPrevActive = hWndTemp;
	}
#endif
	/* FALL THRU!!! */
    default:

#ifdef REMOTECTL
	rc = RemoteWndProc(message,wParam,lParam,&b);
	if (b)
	    return (LONG) rc;
#endif
        return(DefWindowProc(hWnd, message, wParam, lParam));
    }
    return 0L;
}

VOID FAR DoCaption ()
{
    CHAR *sz;

    if (szFileName && szFileName[0]) {
	if (HasWildCard(szFileName))
	    /* can happen with *.tr on command line */
	    sz = szUntitled;
	else {
	    if ((sz = strrchr(szFileName,CHDIRSEPARATOR)) ||
	      (sz = strrchr(szFileName,CHDRIVESEPARATOR)))
		/* never show the path */
		sz++;
	    else
		sz = szFileName;
	}
    }
    else
	sz = szUntitled;
#ifdef WIN2
     sprintf(szScratch, "%s - %s", szCaption,sz);
#else
     wsprintf(szScratch, "%s - %s", (LPSTR) szCaption,(LPSTR) sz);
#endif
     SetWindowText(hWndRecorder, szScratch);
}

LONG APIENTRY KeyWndProc(HWND hWnd, WORD message, WPARAM wParam, LONG lParam)

/* this window exists solely for the purpose of being the target
   of a TranslateMessage call used to xlate virtkeys to ANSI */

{
    return(DefWindowProc(hWnd, message, wParam, lParam));
}

#ifdef WIN2
BOOL WarnEm()
#else
static BOOL NEAR WarnEm()
#endif
/* warn of unsaved changes to current file, return TRUE if OK to continue */
{
    CHAR szTemp[CCHSCRATCH+1];
    INT rc;

    if (bSuspended)
	/* currently handling recording suspension */
	return FALSE;

    if (bWaitActive) {
	/* probably a QUERYENDSESSION while we are trying to tell user
	    something.	Don't die until we get error msg out */
#ifdef NTPORT
	SetForegroundWindow(hWndRecorder);
#else
	SetActiveWindow(hWndRecorder);
#endif
	return FALSE;
    }
    /* definitely finish macro if still recording */
    bSuspendMode = TRUE;
    RecordBreak();
    bSuspendMode = FALSE;
    if (!bFileChanged || (pmFirst == NULL && szFileName == NULL))
	/* no changes, or deleted down to unnamed empty file */
	return TRUE;
    LdStr(hWndRecorder,M_SAVECHANGES);
#ifdef WIN2
    sprintf(szTemp,szScratch,szFileName ? szFileName : szUntitled);
#else
    wsprintf(szTemp,szScratch,
      szFileName ? (LPSTR) szFileName : (LPSTR) szUntitled);
#endif
    if ((rc = MessageBox(hWndRecorder,szTemp,szCaption,MB_YESNOCANCEL))
      == IDYES)

#ifdef NTPORT
	return (BOOL) SendMessage(hWndRecorder,WM_COMMAND,GET_WM_COMMAND_MPS(ID_SAVE,0,0));
#else
	return (BOOL) SendMessage(hWndRecorder,WM_COMMAND,ID_SAVE, 0L);
#endif

    return rc == IDNO;
}

INT FAR MainLoop()
/* main loop of program - WinMain is in init segment */
{
    MSG msg;
    HANDLE hAccel;

    hAccel = LoadAccelerators (hInstRecorder, szRecorder);
    while (GetMessage(&msg, NULL, 0, 0))
	if (!TranslateAccelerator (hWndRecorder, hAccel, &msg)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}

    return (INT)msg.wParam;
}

INT DoDlg(HWND hWndParent,FARPROC lpfn,WORD id)

/* invoke a dialog box */
/* hWndParent: parent to return to */
/* lpfn: dlg box WndProc */

{
    FARPROC lpfnDlg;
    INT rc;
    BOOL b;

    /* no hot keys while recorder is in dlg box */
    b = bHotKeys;
    EnaHotKeys(bHotKeys = FALSE);
    lpfnDlg = MakeProcInstance(lpfn,hInstRecorder);
    rc = DialogBox( hInstRecorder,
		    MAKEINTRESOURCE(id),
		    hWndParent,
		   (FARPROC)lpfnDlg);
    FreeProcInstance (lpfnDlg);
    if (rc == -1) {
	ErrMsg(hWndParent,E_NOMEM);
	rc = 0;
    }
    EnaHotKeys(bHotKeys = b);
    return rc;
}

VOID FreeFile()
{
    FreeMacroList();
    if (szFileName) {
	FreeLocal(szFileName);
	szFileName = 0;
    }
    bFileChanged = FALSE;
    DoCaption();
}

VOID FAR LdStr( HWND hWnd, WORD id)
/* load string from resource file to szScratch */
{
    szScratch[0] = '\0';    /* for safety */
    if (LoadString(hInstRecorder,id,szScratch,CCHSCRATCH - 1) == 0) {
	EnaHotKeys(FALSE);
	MessageBox(hWnd, szErrResource,szCaption,MB_OK|MB_APPLMODAL);
	EnaHotKeys(bHotKeys);
    }
}

VOID PeekABoo()
/* Get Windows into a stable state by draining out our queue and letting
    other apps drain theirs */
{
    MSG msg;
    INT i;
    INT cTimers=0;

    for (i = 0; i < 10; i++ )
	while (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
	    /* have a message - reset i and wait for 10 consecutive
		empties */
	    i = 0;
	    switch(msg.message) {
	    case WM_COMMAND:
	    case WM_SYSCOMMAND:
	    	break;
	    case WM_QUIT:
		return;
	    default:
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_TIMER && ++cTimers > 1)
		    /* give up after 2nd timer tick - else can hang forever
			if machine is really busy */
		    return;
	    }
	}
}

VOID ForceRecActivate(BOOL b)
/* b: TRUE to make recorder active, FALSE to restore previous app */
{
    MSG msg;
    BOOL bOurTimer=FALSE;
static HWND hWnd=0;

    /* make sure Windows stabilizes out any residue from our playback */
    PeekABoo();

    if (!b) {
	if (IsWindow(hWnd) && IsWindowEnabled(hWnd)
	  && !(GetWindowLong(hWnd,GWL_STYLE) & WS_CHILD))
	    /* looks like hWnd is still good (and not another window) */
#ifdef NTPORT
	     SetForegroundWindow(hWnd);
	/* else can't do very much about it */
	else hWnd = 0;	////  If this change fixes anything it was also
			////  a bug in win 3.0 (release's) version. JOHNSP
			////  (basically I added the else)
	return;
#else
	     SetActiveWindow(hWnd);
	/* else can't do very much about it */
	else hWnd = 0;
	return;
#endif
    }
    if (GetActiveWindow() != hWndRecorder) {

#ifdef NTPORT
    //	  if (GetForegroundWindow()) {
	if (GetCapture()) {
#else
	if (GetCapture()) {
#endif
	    if (wTimerID == 0) {
		/* (timer is already running if we are recording) */
		wTimerID = SetTimer(NULL,NULL,FLASHTIMER,(FARPROC)lpfnWndProc);
		bOurTimer = TRUE;
	    }
	    if (wTimerID) {
		/* somebody has mouse capture.  This is very rare, since
		    we always play back whatever buttonup's are necessary on
		    a PB abort.  Can't activate ourselves directly, since
		    mouse owning app may have restricted cursor and will
		    get real confused in any case.  User must get out of capture
		    mode and activate us manually. BUT, if we can't create
		    a timer to flash with, we have no choice but to activate
		    outselves anyway */

		/* make some noise in case we are not visible */
		bWaitActive = TRUE;
		do {
		    if (!GetMessage(&msg, NULL, 0, 0)) {
			/* WM_QUIT - should never happen */
			if (bOurTimer) {
			    KillTimer(NULL,wTimerID);
			    wTimerID = 0;
			}
			return;
		    }
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		} while (GetActiveWindow() != hWndRecorder);
		bWaitActive = FALSE;
		if (bOurTimer) {
		    KillTimer(NULL,wTimerID);
		    wTimerID = 0;
		    /* make sure icon is back to normal */
		    FlashWindow(hWndRecorder,FALSE);
		}
	    }
	}
#ifdef NTPORT
	SetForegroundWindow(hWndRecorder);
#else
	SetActiveWindow(hWndRecorder);
#endif
    }
    /* WM_ACTIVATE caught the hWnd of the window we want to reactivate
	when done */
    hWnd = hWndPrevActive;
}

PMACRO FindPick(INT *pi)
{
    INT i;
    register PMACRO pm;

    if ((i = (INT) SendMessage(hWndList,LB_GETCURSEL,0,0L)) < 0)
	/* no listbox item selected */
	return 0;
    *pi = i;
    for (pm = pmFirst; i > 0 && pm; pm = pm->pmNext, i--)
	;
    return pm;
}

static VOID NEAR SearchAndDestroy()
{
    PMACRO pm;
    INT i;
    CHAR szTemplate[CCHSCRATCH];
    CHAR szTemp[CCHSCRATCH];

    if (pm = FindPick(&i)) {
	/* get sprintf template */
	LoadString(hInstRecorder,M_DELETEQ,szTemplate,CCHSCRATCH - 1);
	if (pm->f.hotkey)
	    /* get hotkey string in szScratch */
	    HotKeyString(pm,FALSE);
	else
	    strcpy(szScratch,pm->f.szComment);
#ifdef WIN2
	sprintf(szTemp,szTemplate,szScratch);
#else
	wsprintf(szTemp,szTemplate,(LPSTR) szScratch);
#endif
	if (MessageBox(hWndRecorder,szTemp,szCaption,
	  MB_ICONEXCLAMATION|MB_OKCANCEL) != IDOK)
	    return;
	DeleteMacro(pm);
    }
}

VOID DeleteMacro(PMACRO pm)
{
    PMACRO pmPrev;
    INT rc,i;

    i = 0;
    if (pm == pmFirst)
	pmFirst = pm->pmNext;
    else {
	pmPrev = pmFirst;
	while (1) {
	    i++;
	    if (pmPrev->pmNext == pm)
		break;
	    pmPrev = pmPrev->pmNext;
	}
	pmPrev->pmNext = pm->pmNext;
    }
    FreeMacro(pm,TRUE);
    SendMessage(hWndList,LB_DELETESTRING,i,0L);
    rc = (INT) SendMessage(hWndList,LB_GETCOUNT,0,0L);
    bFileChanged = TRUE;
    if (rc > 0)
	/* select next macro */
	SendMessage(hWndList,LB_SETCURSEL,(WORD) min(i,rc - 1),0L);
    else {
	EnableWindow(hWndList,FALSE);
	SetFocus(hWndRecorder);
    }
}

#ifdef NTPORT
HWND GetForegroundWindow( VOID ) {

#ifdef NTPORT
OutputDebugString("START: GetForegroundWindow\n\r");
#endif


    return GetWindow(GetDesktopWindow(),GW_CHILD);
}
#endif
/* ------------------------------ EOF --------------------------------- */
