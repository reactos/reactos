
/***********************************************************************
NAME

    dlg.c -- Recorder dialog boxes

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
		9 Dec		msg display now includes line number
				edit can now remove hot key (bug fix)
				now displays message/dialogbox class
				 instead of #32xxx nonsense
		14 Dec		fixed listbox hilite bug on deleting
				 last macro in LB
				New MsgString prettifications.
				Dblclk on macro/play listbox now works
		16 Dec		Fixed bug not recognizing no selection
				 in dlgPick listbox
		19 dec		now using PMACRO typedef
		20 Dec		dlgAbout now handles DLGTRACEROK for
				 pb abort message. No IDCANCEL support.
				Fixed macro edit bug
				Now handles undisplayable macro event list
				 gracefully
		22 Dec		Adapted for combo boxes with Win30
		28 Dec		Adapted for SBCombo
		29 Dec		Fixed dlgEdit bug
		30 Dec		now reads combo box results correctly
		3 Jan 89	more sensible control id's
				dlgRecord now has empty hot key combobox
			  	edit field
		5 Jan		delete of last macro now clears file too
				renamed Profile to Pref
				added dlgInterrupt
		6 Jan		made some internal funcs static
				now use CenterMe
		9 Jan		msec field of displayed macro now a long
		11 Jan		upgrade to use newly working Win3 comboboxes
		12 Jan		dlgPref now displays autoLOAD file
		13 Jan		improved dlgInterrupt
		30 Jan		fixed PBError dlg box bug
		2 Feb		now deals with system font at record time
		3 Feb		PBerror now notes bad screen/font
				  only if macro has mouse msgs
		24 Feb 89	cleared out some old ifdef's
		10 Mar 89	macro commands now in separate dlg box
		13 Mar 89	restored Win2.1 support
		15 Mar 89	dlgPref now actually saves flags in WIN.INI
				now passing junk buff to ToAscii
		16 Mar 89	improved error reporting
		17 Mar 89	now honors dblclk on record interrupt radio
    sds		25 Mar 1989	MsgString now NEVER reads beyond segment end
				better oversize dlgCmds handling
    sds		29 Mar 1989	Now selecting first event in dlgCmds
				pref/autoload now saves full file path
    sds		30 Mar 1989	Now disallows ctrl/alt hot keys until
				they actually work in Windows
				LdStr with hWnd arg
    sds		31 Mar 1989	bug fix in dlgPref
    sds		6 Apr 1989	now clearing state buffer for ToAscii
    sds		26 Apr 1989	deals with B_FORCEACTIVE bit
    sds		27 Apr 1989	different break types for int dlg box
				new desktop naming for event display
    sds		28 Apr 1989	goes to hourglass cursor faster on editmacro
    sds		5 May 1989	now using DESKTOPMODULE magic
    sds		9 May 1989	now using window to xlate virtkey to ANSI
    sds		11 May 1989	WIN2 fix
    sds		13 May 1989	now minimizes on playback from dlgbox
    sds		16 May 1989	fix for discarding macro code
		18 May 1989	massive user intfc changes
    sds		22 May 1989	fixed hotkey display in PBError (win3)
    sds		23 May 1989	fixed bug in not loading macro code
    sds		30 May 1989	system menu support
    sds		31 May 1989	CLOSE record suspension now resumes
    sds		7 Jun 1989	now cleaning out system menu, no
				SC_CLOSE needed
    sds		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	WIN2 changes
				now deleting sysmenu separators
    sds		3 Jul 1989	FixSysMenu no longer needed for Win30
			    	now using wsprintf
    sds		27 Jul 1989	WriteProfile to szRecorder, not szCaption
    sds		8 Aug 1989	MsgString now clears SHIFT for TranslateMsg
    sds		17 Aug 1989	error dlgbox now supports save on TOOLONG
    sds		17 Aug 1989	AddHotKey now takes id
    sds		28 Aug 1989	fixed bug displaying wrong instruction number
				for Desktop Msg
    sds		20 Sep 1989	ABOUT now comes down on CANCEL
				removed bogus GetKeyNametext call
    sds		21 Sep 1989	better handling of pb error during recording
    sds		22 Sep 1989	MsgString now clears ALT and CTRL before
				  TranslateMsg - cleaner xlate
    sds		30 Oct 1989	fixed bug losing hotkey
    sds		31 Oct 1989	fixed bug with B_NESTS flag
    sds		6 Dec 1989	altered wait cursor handling in dlgEdit
    sds		19 Dec 1989	now deletes CLOSE option on dlgInt sysmenu
    sds		26 Dec 1989	now checks for duplicate comments
    sds		1 Feb 1990	fixed check for duplicate comments
    sds 	8 Feb 1990	Win2.1 compatibility
    sds 	15 Mar 1990	Win2.1 compatibility
    sds 	26 Oct 1990	bogus lParam references for -W3
    sds 	29 Oct 1990	fixed errordlg - no gray text
				ignore mouse grays Win/Scr relative
    sds 	8 Jan 1991	fixed bug in MsgString

*/
 
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */

#include <windows.h>
#include <port1632.h>
#undef min
#undef max
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#ifdef WIN2
#include "controls.h"
#endif
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */
static VOID NEAR CenterMe(HWND hWnd);
static INT NEAR DispMacro(PMACRO pm,HWND hWnd);
static VOID NEAR GetMouseMode(HWND hDlg);
static WORD NEAR GetRecordOpts(HWND hDlg);

#ifdef NTPORT
static INT NEAR MsgString(LPRECORDERMSG,LPRECORDERMSG);
#else
static INT NEAR MsgString(LPRECORDERMSG);
#endif
static BOOL NEAR NewMacro(HWND hDlg);
static VOID NEAR SetMouseOpt(HWND hDlg);
static VOID NEAR SetPlayOpt(HWND hDlg);

/* VARIABLES DEFINED */
static WORD newflags;
static HANDLE hMemDesc=0;
static WORD cchDesc=0;

/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */
BOOL APIENTRY dlgRecord(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    BOOL b;

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	CheckDlgButton(hDlg,IDCONTROL,TRUE);
	ListKeyCodes(GetDlgItem(hDlg,IDHOTKEY));
	/* current hot key selection should be blank - let users hit combo
	   box if they want to */
	SendDlgItemMessage(hDlg,IDHOTKEY,CB_SETEDITSEL,0,
	  (LONG) (LPSTR) szNull);
	MouseMode = DefMouseMode;
	newflags = DefFlags;
	SetMouseOpt(hDlg);
	SetPlayOpt(hDlg);
	SendDlgItemMessage(hDlg,IDEDIT,EM_LIMITTEXT,CCHCOMMENT,0L);
	break;

    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif

#ifdef WINDOWSBUG

	/* ctrl+alt[+shift] keys don't show up at keyboard hook correctly,
	    so disallow that combination */

	case IDCONTROL:
	case IDALT:
	    if (IsDlgButtonChecked(hDlg,IDCONTROL) &&
	      IsDlgButtonChecked(hDlg,IDALT))
#ifdef NTPORT
		CheckDlgButton( hDlg,
				GET_WM_COMMAND_ID(wParam,lParam)==IDALT ? IDCONTROL : IDALT,
				FALSE);
#else
		CheckDlgButton(hDlg,wParam == IDALT ? IDCONTROL : IDALT,FALSE);
#endif
	    break;

#endif
	case IDMOUSEOPT:
	    if (SendDlgItemMessage(hDlg,IDMOUSEOPT,CB_GETCURSEL,0,0L) == 0)
		/* no mouse - gray out Win/Screen relative */
		b = FALSE;
	    else
		b = TRUE;
	    EnableWindow(GetDlgItem(hDlg,IDTEXT),b);
	    EnableWindow(GetDlgItem(hDlg,IDWINRELATIVE),b);
	    break;

	case IDOK:
	    if (NewMacro(hDlg))
		EndDialog (hDlg,TRUE);
	    /* actually start up from main WndProc to get activations
		right */
	    break;
	case IDCANCEL:
	    EndDialog (hDlg,FALSE);
	    break;
	default:
	    return FALSE;
	}
	break;
    default:
	return FALSE;
    }
    return TRUE;
    lParam;	/* -W3	*/
}

static BOOL NEAR NewMacro(HWND hDlg)
{
    INT rc;
    HWND hWnd;

    if (pmNew)
	FreeMacro(pmNew,FALSE);
    if ((pmNew = AllocLocal(sizeof(struct macro))) == 0) {
	ErrMsg(hDlg,E_NOMEM);
	return FALSE;
    }

    if (SendMessage(hWnd=GetDlgItem(hDlg,IDDESC),EM_GETMODIFY,0,0L)) {
	/* data was entered */
	if ((pmNew->f.cchDesc = GetWindowTextLength(hWnd)) > 0) {
	    if ((pmNew->hMemDesc = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,(LONG)++pmNew->f.cchDesc)) == 0) {
		ErrMsg(hDlg,E_NOMEM);
		SetFocus(GetDlgItem(hDlg,IDDESC));
		return FALSE;
	    }
	    GetWindowText(hWnd,GlobalLock(pmNew->hMemDesc),pmNew->f.cchDesc);
	    GlobalUnlock(pmNew->hMemDesc);
	}
    }

    pmNew->f.flags = newflags;
    if (IsDlgButtonChecked(hDlg,IDCONTROL))
	pmNew->f.flags |= B_CONTROL;
    if (IsDlgButtonChecked(hDlg,IDSHIFT))
	pmNew->f.flags |= B_SHIFT;
    if (IsDlgButtonChecked(hDlg,IDALT))
	pmNew->f.flags |= B_ALT;
    if (IsDlgButtonChecked(hDlg,IDLOOP))
	pmNew->f.flags |= B_LOOP;
    if (IsDlgButtonChecked(hDlg,IDCHECK))
	pmNew->f.flags |= B_NESTS;
    else
	pmNew->f.flags &= ~B_NESTS;

    pmNew->f.hotkey = '\0';

    szScratch[0] = '\0';
    GetDlgItemText(hDlg,IDHOTKEY,szScratch,CCHSCRATCH - 1);
    if (strlen(szScratch) > 0) {
	if (rc =(UCHAR)GetKeyCode(szScratch)) {
	    if (!CheckHotKey((UCHAR) rc,pmNew->f.flags,NULL)) {
		ErrMsg(hDlg,E_DUPHOTKEY);
		SetFocus(GetDlgItem(hDlg,IDHOTKEY));
		return FALSE;
	    }
	    pmNew->f.hotkey =(UCHAR)rc;
	}
	else {
	    ErrMsg(hDlg,E_BADHOTKEY);
	    SetFocus(GetDlgItem(hDlg,IDHOTKEY));
	    return FALSE;
	}
    }

    pmNew->f.flags &= ~(B_RECSPEED|B_WINRELATIVE|B_PLAYTOWINDOW);
    pmNew->f.flags |= GetRecordOpts(hDlg);
    GetMouseMode(hDlg);

    if (MouseMode == RM_NOTHING)
	/* no mouse is win relative, for consistency */
	pmNew->f.flags |= B_WINRELATIVE;

    if (GetDlgItemText(hDlg,IDEDIT,szScratch,CCHCOMMENT) > 0)
	/* already NULL'ed out, just strlen */
	memcpy(pmNew->f.szComment,szScratch,
	  max(strlen(szScratch),CCHCOMMENT - 1));
    if (pmNew->f.hotkey == '\0' && pmNew->f.szComment[0] == '\0') {
	ErrMsg(hDlg,E_KEYORCOMMENT);
	SetFocus(GetDlgItem(hDlg,IDHOTKEY));
	return FALSE;
    }
    if (pmNew->f.hotkey == 0 && !CheckComment(pmNew,pmNew->f.szComment)) {
	ErrMsg(hDlg,E_DUPCOMMENT);
	SetFocus(GetDlgItem(hDlg,IDEDIT));
	return FALSE;
    }

    return TRUE;
}

static WORD NEAR GetRecordOpts(HWND hDlg)
/* return flags word with RECSPEED, WINRELATIVE, PLAYTOWINDOW set */
{
    WORD f;

    if (SendDlgItemMessage(hDlg,IDRECSPEED,CB_GETCURSEL,0,0L) == 1)
	f = B_RECSPEED;
    else
	f = 0;
    if (SendDlgItemMessage(hDlg,IDWINRELATIVE,CB_GETCURSEL,0,0L) == 1)
	f |= B_WINRELATIVE;
    if (SendDlgItemMessage(hDlg,IDPLAYTO,CB_GETCURSEL,0,0L) == 1)
	f |= B_PLAYTOWINDOW;
    return f;
}

static VOID NEAR GetMouseMode(HWND hDlg)
{
    switch ((WORD) SendDlgItemMessage(hDlg,IDMOUSEOPT,CB_GETCURSEL,0,0L)) {
    case 0:
	MouseMode = RM_NOTHING;
	break;
    case 1:
	MouseMode = RM_EVERYTHING;
	break;
    default:
	MouseMode = RM_DRAGS;
    }
}

static VOID NEAR SetMouseOpt(HWND hDlg)
{
    INT i;
    HWND hWnd;

    hWnd = GetDlgItem(hDlg,IDMOUSEOPT);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG)(LPSTR)szNothing);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG)(LPSTR)szEverything);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG)(LPSTR)szClickDrag);

    switch(MouseMode) {
    case RM_NOTHING:
	i = 0;
	break;
    case RM_EVERYTHING:
	i = 1;
	break;
    case RM_DRAGS:
	i = 2;
    }
    SendMessage(hWnd,CB_SETCURSEL,(WORD)i,0L);

    hWnd = GetDlgItem(hDlg,IDWINRELATIVE);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG)(LPSTR)szScreen);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG)(LPSTR)szWindow);
    SendMessage(hWnd,CB_SETCURSEL,(newflags & B_WINRELATIVE) ? 1 : 0,0L);
}

static VOID NEAR SetPlayOpt(HWND hDlg)
{
    HWND hWnd;

    hWnd = GetDlgItem(hDlg,IDPLAYTO);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG) (LPSTR) szAnyWindow);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG) (LPSTR) szSameWindow);
    SendMessage(hWnd,CB_SETCURSEL,(newflags & B_PLAYTOWINDOW) ? 1 : 0,0L);

    hWnd = GetDlgItem(hDlg,IDRECSPEED);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG) (LPSTR) szFast);
    SendMessage(hWnd,CB_ADDSTRING,0,(LONG) (LPSTR) szRecSpeed);
    SendMessage(hWnd,CB_SETCURSEL,(newflags & B_RECSPEED) ? 1 : 0,0L);
    CheckDlgButton(hDlg,IDLOOP,newflags & B_LOOP);
    CheckDlgButton(hDlg,IDCHECK,newflags & B_NESTS);
}

BOOL APIENTRY dlgEdit(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    LPRECORDERMSG lpt;
    HANDLE hMem;
    HWND hWnd;
    WORD cch;
    WORD flags;
    BYTE hotkey;
    CHAR szTemp[CCHSCRATCH];
    INT rc;

    switch(msg)	{
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	SetCursor(hCurWait);
	if ((rc = LoadDesc(pmEdit)) == 0)
	    /* load description edit box */
	    if (pmEdit->hMemDesc) {
		SetDlgItemText(hDlg,IDDESC,GlobalLock(pmEdit->hMemDesc));
		GlobalUnlock(pmEdit->hMemDesc);
	    }

	/* load keycodes into combo box as well */
	ListKeyCodes(GetDlgItem(hDlg,IDHOTKEY));
	SetCursor(hCurNormal);
	if (rc) {
	    hWnd = GetParent(hDlg);
	    EndDialog(hDlg,FALSE);
	    ErrMsgArg(hWnd,rc,szErrArg);
	    break;
	}


	if (pmEdit->f.hotkey) {
	    GetKeyString(pmEdit->f.hotkey,szScratch);
	    if (strlen(szScratch) == 1)
		SetDlgItemText(hDlg,IDHOTKEY,szScratch);
	    else
		/* string is actually in listbox */
		SendDlgItemMessage(hDlg,IDHOTKEY,CB_SELECTSTRING,TRUE,
		  (LONG)(LPSTR)szScratch);
	}
	memcpy(szScratch,pmEdit->f.szComment,CCHCOMMENT);
	szScratch[CCHCOMMENT] = '\0';
	SetDlgItemText(hDlg,IDEDIT,szScratch);
	SendDlgItemMessage(hDlg,IDEDIT,EM_LIMITTEXT,CCHCOMMENT,0L);

	CheckDlgButton(hDlg,IDSHIFT,pmEdit->f.flags & B_SHIFT);
	CheckDlgButton(hDlg,IDALT,pmEdit->f.flags & B_ALT);
	CheckDlgButton(hDlg,IDCONTROL,pmEdit->f.flags & B_CONTROL);

	newflags = pmEdit->f.flags;
	SetPlayOpt(hDlg);

	if (pmEdit->f.cxScreen == 0) {
	    LdStr(hDlg,M_NOMOUSE);
	    SetDlgItemText(hDlg,IDTEXT,szScratch);
	}
	else {
	    if (pmEdit->f.cxScreen == cxScreen
	      && pmEdit->f.cyScreen == cyScreen)
		LdStr(hDlg,M_RIGHTSCREEN);
	    else
		LdStr(hDlg,M_WRONGSCREEN);
#ifdef WIN2
	    sprintf(szTemp,szScratch,pmEdit->f.cxScreen,pmEdit->f.cyScreen);
#else
	    wsprintf(szTemp,szScratch,pmEdit->f.cxScreen,pmEdit->f.cyScreen);
#endif
	    SetDlgItemText(hDlg,IDTEXT,szTemp);
	}

	SetDlgItemText(hDlg,IDWINRELATIVE,(pmEdit->f.flags & B_WINRELATIVE)
	  ? (LPSTR) szWindow : (LPSTR) szScreen);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif

#ifdef WINDOWSBUG
	case IDCONTROL:
	case IDALT:
	    if (IsDlgButtonChecked(hDlg,IDCONTROL) &&
	      IsDlgButtonChecked(hDlg,IDALT))
#ifdef NTPORT
		CheckDlgButton(hDlg,GET_WM_COMMAND_ID(wParam,lParam)== IDALT ? IDCONTROL : IDALT,FALSE);
#else
		CheckDlgButton(hDlg,wParam == IDALT ? IDCONTROL : IDALT,FALSE);
#endif
	    break;
#endif
	case IDOK:
	    flags = pmEdit->f.flags;
	    szScratch[0] = '\0';
	    rc = GetDlgItemText(hDlg,IDHOTKEY,szScratch,CCHSCRATCH - 1);
	    if (rc > 0) {
		if (rc = (UCHAR) GetKeyCode(szScratch))
		    hotkey = (UCHAR) rc;
		else {
		    ErrMsg(hDlg,E_BADHOTKEY);
		    SetFocus(GetDlgItem(hDlg,IDHOTKEY));
		    break;
		}

		flags &= ~(B_SHFT_CONT_ALT);
		if (IsDlgButtonChecked(hDlg,IDSHIFT))
		    flags |= B_SHIFT;
		if (IsDlgButtonChecked(hDlg,IDCONTROL))
		    flags |= B_CONTROL;
		if (IsDlgButtonChecked(hDlg,IDALT))
		    flags |= B_ALT;
	    }
	    else {
		/* no hot key, no shift bits */
		hotkey = 0;
		flags &= ~(B_SHFT_CONT_ALT);
	    }

	    flags &= ~(B_NESTS|B_LOOP|B_RECSPEED|B_PLAYTOWINDOW);
	    if (IsDlgButtonChecked(hDlg,IDCHECK))
		flags |= B_NESTS;
	    if (IsDlgButtonChecked(hDlg,IDLOOP))
		flags |= B_LOOP;
	    if (SendDlgItemMessage(hDlg,IDRECSPEED,CB_GETCURSEL,0,0L) == 1)
		flags |= B_RECSPEED;
	    if (SendDlgItemMessage(hDlg,IDPLAYTO,CB_GETCURSEL,0,0L) == 1)
		flags |= B_PLAYTOWINDOW;

	    GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH);
	    if (szScratch[0] == '\0' && hotkey == '\0') {
		ErrMsg(hDlg,E_KEYORCOMMENT);
		SetFocus(GetDlgItem(hDlg,IDHOTKEY));
		break;
	    }

	    if (hotkey == 0 && !CheckComment(pmEdit,szScratch)) {
		ErrMsg(hDlg,E_DUPCOMMENT);
		SetFocus(GetDlgItem(hDlg,IDEDIT));
		return FALSE;
	    }

	    if (flags != pmEdit->f.flags
	      || pmEdit->f.hotkey != hotkey) {
		if (!CheckHotKey(hotkey,flags,pmEdit)) {
		    ErrMsg(hDlg,E_DUPHOTKEY);
		    SetFocus(GetDlgItem(hDlg,IDHOTKEY));
		    break;
		}

		if (pmEdit->f.hotkey)
		    DelHotKey(pmEdit->f.hotkey,pmEdit->f.flags);
		pmEdit->f.flags = flags;
		if (pmEdit->f.hotkey = hotkey)
		    if (!AddHotKey(pmEdit->f.hotkey,pmEdit->f.flags,
		      pmEdit->id))
			ErrMsg(hDlg,E_TOOMANYHOT);
	    }

	    if (SendMessage(hWnd = GetDlgItem(hDlg,IDDESC),
	      EM_GETMODIFY,0,0L)) {
		/* description was modified */
		cch = GetWindowTextLength(hWnd);

		if (cch > 0 && (hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,(LONG)++cch))==0)
		    ErrMsg(hDlg,E_NOMEM);
		else {
		    if (pmEdit->hMemDesc)
			GlobalFree(pmEdit->hMemDesc);
		    if (cch > 0) {
			GetWindowText(hWnd,GlobalLock(hMem),cch);
			GlobalUnlock(pmEdit->hMemDesc = hMem);
		    }
		    else
			pmEdit->hMemDesc = 0;
		    pmEdit->f.cchDesc = cch;
		}
	    }
	    strncpy(pmEdit->f.szComment,szScratch,CCHCOMMENT);

	    /* update code segment too */
	    if (rc = LoadMacro(pmEdit))
		ErrMsg(hDlg,rc);
	    else {
		lpt = (LPRECORDERMSG) GlobalLock(pmEdit->hMemCode);
		lpt->message = (WORD) pmEdit->f.flags;
		GlobalUnlock(pmEdit->hMemCode);
		ReleaseCode(pmEdit);
	    }

	    bFileChanged = TRUE;
	    /* FALL THRU!!! */
	case IDCANCEL:

#ifdef NTPORT
	    EndDialog (hDlg,GET_WM_COMMAND_ID(wParam,lParam)==IDOK);
#else
	    EndDialog (hDlg,wParam == IDOK);
#endif
	    break;
	default:
	    return FALSE;
	}
	break;
    default:
	return FALSE;
    }
    return TRUE;
    lParam; /* -W3 */
}

BOOL APIENTRY dlgCmds(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    INT rc;
    HWND hWnd;

    switch(msg)	{
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	/* load macro listbox */
	SetCursor(hCurWait);
	/* do other stuff BEFORE calling DispMacro, as unintuitive as
	    this may seem.  If cmds listbox runs out of memory, we can't load
	    a resource string either.  No idea why */
	HotKeyString(pmEdit,FALSE);
	SetDlgItemText(hDlg,IDTEXT,szScratch);
	rc = DispMacro(pmEdit,GetDlgItem(hDlg,IDLIST));
	SetCursor(hCurNormal);
	if (rc) {
	    hWnd = GetParent(hDlg);
	    EndDialog(hDlg,FALSE);
	    ErrMsgArg(hWnd,rc,szErrArg);
	    break;
	}
	SendDlgItemMessage(hDlg,IDLIST,LB_SETCURSEL,0,0L);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch(GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch(wParam) {
#endif
	case IDOK:
	case IDCANCEL:
	    EndDialog(hDlg,TRUE);
	    break;
	default:
	    return FALSE;
	}
	break;
    default:
	return FALSE;
    }
    return TRUE;
    lParam; /* -W3 */
}

BOOL APIENTRY dlgAbout(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
/* NOTE: this function is also used for DLGRECORDEROK, as a stub to
    merely recognize an OK button.  DO NOT take DLGRECORDEROK down on
    IDCANCEL, since we put up the Playback Aborted message here, which was
    caused by a ctrl/break detected through async key state but not
    actually read from queue.  It comes out of the queue to this WndProc,
    where we discard it */
{
    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	if (bCenterDlg)
	    /* need to center */
	    CenterMe(hDlg);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif
	case IDCANCEL:
	    if (bCenterDlg)
		/* not an ABOUT box - forget it */
		break;
	case IDOK:
	    EndDialog (hDlg,TRUE);
	    break;
	default:
	    return FALSE;
	}
	/* FALL THRU to Break */
    default:
	return FALSE;
    }
    return TRUE;
    lParam; /* -W3 */
}

BOOL APIENTRY dlgInterrupt(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
/* macro record interrupt */
{
#ifdef NTPORT
WPARAM wP;
#endif

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#else
#ifdef WIN32
	DeleteMenu( GetSystemMenu(hDlg,FALSE),
		    SC_CLOSE,
		    MF_BYCOMMAND);
#else
	ChangeMenu( GetSystemMenu(hDlg,FALSE),
		    SC_CLOSE,
		    NULL,
		    NULL,
		    MF_DELETE);
#endif
#endif
	/* need to center */
	CenterMe(hDlg);
	CheckRadioButton(hDlg,IDSAVE,IDTRASH,IDRESUME);
	switch(fBreakType) {
	case 0:		/* real break */
	    SetFocus(GetDlgItem(hDlg,IDRESUME));
	    break;
	case E_TOOLONG:	    /* macro too long - allow save or trash */
	    LoadString(hInstRecorder,M_TOOLONG,szScratch,CCHSCRATCH);
	    SetDlgItemText(hDlg,IDTEXT,szScratch);
	    EnableWindow(GetDlgItem(hDlg,IDRESUME),FALSE);
	    SetFocus(GetDlgItem(hDlg,IDTRASH));
	    break;
	case M_PBERECORD:   /* playback error during recording */
	case M_BREAKRECORD: /* playback cancel during recording */
	    LoadString(hInstRecorder,fBreakType,szScratch,CCHSCRATCH);
	    SetDlgItemText(hDlg,IDTEXT,szScratch);
	    SetFocus(GetDlgItem(hDlg,IDTRASH));
	    break;
	case E_JRSYSMODAL:  /* sysmodal dlg box encountered */
	    LoadString(hInstRecorder,M_SYSMODAL,szScratch,CCHSCRATCH);
	    SetDlgItemText(hDlg,IDTEXT,szScratch);
	    /* FALL THRU */
	case 1:		/* Recorder activated */
	    SetFocus(GetDlgItem(hDlg,IDSAVE));
	}
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif
	case IDSAVE:
	case IDRESUME:
	case IDTRASH:
#ifdef NTPORT
	    CheckRadioButton(hDlg,IDSAVE,IDTRASH,GET_WM_COMMAND_ID(wParam,lParam));
	    if (GET_WM_COMMAND_CMD(wParam,lParam)!= BN_DOUBLECLICKED)
		break;
#else
	    CheckRadioButton(hDlg,IDSAVE,IDTRASH,wParam);
	    if (HIWORD(lParam) != BN_DOUBLECLICKED)
		break;
#endif
	    /* FALL THRU TO END DLG BOX */
#ifdef NTPORT
	case IDOK:
	    if (IsDlgButtonChecked(hDlg,IDSAVE))
		wP= IDSAVE;
	    else if (IsDlgButtonChecked(hDlg,IDRESUME))
		wP= IDRESUME;
	    else
		wP= IDTRASH;
	    EndDialog (hDlg,wP);
	    break;
#else
	case IDOK:
	    if (IsDlgButtonChecked(hDlg,IDSAVE))
		wParam = IDSAVE;
	    else if (IsDlgButtonChecked(hDlg,IDRESUME))
		wParam = IDRESUME;
	    else
		wParam = IDTRASH;
	    EndDialog (hDlg,wParam);
	    break;
#endif
	default:
	    return FALSE;
	}
	/* FALL THRU to Break */
    default:
	return FALSE;
    }
    return TRUE;
}

BOOL APIENTRY dlgPref(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    CHAR szTemp[40];

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	newflags = DefFlags;
	MouseMode = DefMouseMode;
	SetMouseOpt(hDlg);
	SetPlayOpt(hDlg);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif
	case IDOK:
	    newflags &= ~(B_RECSPEED|B_WINRELATIVE|B_PLAYTOWINDOW);
	    newflags |= GetRecordOpts(hDlg);
	    _itoa(newflags,szTemp,10);
	    WriteProfileString(szRecorder,szPlayback,szTemp);

	    GetMouseMode(hDlg);
	    _itoa(MouseMode,szTemp,10);
	    WriteProfileString(szRecorder,szRecord,szTemp);

	    DefMouseMode = MouseMode;
	    DefFlags = newflags;
	    /* FALL THRU */
	case IDCANCEL:
	    EndDialog (hDlg,TRUE);
	    break;
	default:
	    return FALSE;
	}
	break;
    default:
	return FALSE;
    }
    return TRUE;
    lParam; /* -W3 */
}

static INT NEAR DispMacro(PMACRO pm,HWND hWnd)
/* display current macro in listbox at hWnd */
{
    WORD rc;
    register INT i;

    LPRECORDERMSG lpt,lptS;

    if (rc = LoadMacro(pm))
	return rc;

    lpt = (LPRECORDERMSG) GlobalLock(pm->hMemCode);

#ifdef NTPORT
    lptS=lpt;
#endif

    /* cMsgs is in paramL */
    i = lpt->paramL - 1;
    lpt++;
    for (; i > 0 && rc == 0; i--, lpt++) {

#ifdef NTPORT
	if ((rc = MsgString(lpt,lptS)) == 0)
#else
	if ((rc = MsgString(lpt) == 0)
#endif
	    if (SendMessage(hWnd,LB_ADDSTRING,0,(LONG) (LPSTR) szScratch)
	      < 0) {
		/* macro too big to load - delete last two lines and
		    put error msg at end */
		rc = (WORD) SendMessage(hWnd,LB_GETCOUNT, 0, 0L);
		SendMessage(hWnd,LB_DELETESTRING, --rc, 0L);
		SendMessage(hWnd,LB_DELETESTRING, --rc, 0L);
		SendMessage(hWnd,LB_ADDSTRING, 0, (LONG) (LPSTR) szNoMem);
		rc = 0;
		break;
	    }
    }
    GlobalUnlock(pm->hMemCode);
    ReleaseCode(pm);
    return rc;
}
#ifdef NTPORT
INT NEAR MsgString(LPRECORDERMSG lpt, LPRECORDERMSG lptS )
/* write string for message into szScratch */
{
#else
INT NEAR MsgString(LPRECORDERMSG lpt)
/* write string for message into szScratch */
{
#endif

    CHAR *sz,*szW;
    CHAR szWin[31];
    CHAR szKey[20]; /* unlikely to have a keyname string this LONG */
    CHAR rgchKeys[256];
    MSG msg;
    LPSTR lpch;
    BOOL bKey;
    BYTE ch,chShift,chCtrl,chAlt;
#ifndef NTPORT
    WORD x;
#endif
    INT i,cch;

    bKey = FALSE;
    switch(lpt->message & ~(B_FORCEACTIVE|B_RESTORE)) {
    case WM_LBUTTONDOWN:
	sz = rgszMsg[MSG_LDOWN - MSG_LDOWN];
	break;
    case WM_MBUTTONDOWN:
	sz = rgszMsg[MSG_MDOWN - MSG_LDOWN];
	break;
    case WM_RBUTTONDOWN:
	sz = rgszMsg[MSG_RDOWN - MSG_LDOWN];
	break;
    case WM_LBUTTONUP:
	sz = rgszMsg[MSG_LUP - MSG_LDOWN];
	break;
    case WM_MBUTTONUP:
	sz = rgszMsg[MSG_MUP - MSG_LDOWN];
	break;
    case WM_RBUTTONUP:
	sz = rgszMsg[MSG_RUP - MSG_LDOWN];
	break;
    case WM_MOUSEMOVE:
	sz = rgszMsg[MSG_MOVE - MSG_LDOWN];
	break;
    case WM_KEYDOWN:
	sz = rgszMsg[MSG_KDOWN - MSG_LDOWN];
	bKey = TRUE;
	break;
    case WM_KEYUP:
	sz = rgszMsg[MSG_KUP - MSG_LDOWN];
	bKey = TRUE;
	break;
    case WM_SYSKEYDOWN:
	sz = rgszMsg[MSG_SKDOWN - MSG_LDOWN];
	bKey = TRUE;
	break;
    case WM_SYSKEYUP:
	sz = rgszMsg[MSG_SKUP - MSG_LDOWN];
	bKey = TRUE;
	break;
    default:
	/* should never happen */
	return E_BADMACRO;
    }


#ifdef NTPORT
    lpch=(LPSTR)(lpt->u.szWindow+(LONG)lptS);
    i = (INT)((LONG)lptS-(LONG)lpt);	     /* offset in bytes into mem area */
    i = i/sizeof(RECORDERMSG);		     /* number of messages */
    if (i<0) i=i*-1;
#else
    x = HIWORD(lpt);
    lpch = (LPSTR) MAKELONG(lpt->u.szWindow,x);

    /* instruction number is implicit in offset */
    i = LOWORD((DWORD)(lpt))/sizeof(RECORDERMSG);
#endif

#ifndef WIN2
    if (*lpch == DESKTOPMODULE) {
	strcpy(szWin,szWindows);
	strcat(szWin,"!");
	strcat(szWin,szDesktop);
    }
    else {
#endif
	if ((cch = lstrlen(lpch) + 1) > 29)
	    cch = 29;
	lmemcpy(szWin,lpch,cch);
	szWin[cch] = '\0';
	szW = strrchr(szWin,'!');

	if (szW) {
	    /* translate weird internal names to something more friendly */
	    szW++;
	    if (strcmp(szW,szDlgInt) == 0)
		strcpy(szW,szDialogBox);
#ifdef WIN2
	    if (strncmp(szWin,szWin200,strlen(szWin200)) == 0) {
		if (_strcmpi(szW,szDesktop) == 0)
		    strcpy(szScratch,szWindows);
		else
		    strcpy(szScratch,szMSDOS);
		strcat(szScratch,szW - 1);
		strcpy(szWin,szScratch);
		szW = strrchr(szWin,'!') + 1;
	    }
	    if (strcmp(szW,szMsgInt) == 0)
		strcpy(szW,szMessageBox);
#endif
	}
#ifndef WIN2
    }
#endif

    if (bKey) {
	/* this is really disgusting.  There is no direct documented
	   way to translate virtual keys to ANSI.  We therefore create
	   a bogus KEYDOWN and use TranslateMessage to do the xlate
	   for us, using the resulting WM_CHAR as the ANSI value. */
	GetKeyboardState(rgchKeys);
	chShift = rgchKeys[VK_SHIFT];
	chAlt = rgchKeys[VK_MENU];
	chCtrl = rgchKeys[VK_CONTROL];
	/* clear out shift keys, else get wrong codes back */
	rgchKeys[VK_SHIFT] = 0;
	rgchKeys[VK_MENU] = 0;
	rgchKeys[VK_CONTROL] = 0;
	SetKeyboardState(rgchKeys);
	msg.hwnd = hWndKey;
	msg.message = WM_KEYDOWN;
	/* virt key in wParam */
	msg.wParam = (WORD) LOBYTE(lpt->paramL);
	ch = LOBYTE(msg.wParam);
	/* OEM scan code in low byte of high word of lParam */
	msg.lParam = MAKELONG(1,(WORD) HIBYTE(lpt->paramL));
	if (TranslateMessage(&msg)) {
	    /* was xlated, get the WM_CHAR message */
	    do {
		if (PeekMessage(&msg,hWndKey,NULL,NULL,PM_REMOVE))
		    /* process it, especially if important */
		    DispatchMessage(&msg);
		else
		    /* nothing in queue - TranslateMessage lies sometimes */
		    bKey = FALSE;
	    } while (bKey && msg.message != WM_CHAR &&
	      msg.message != WM_SYSCHAR && msg.message != WM_DEADCHAR
	      && msg.message != WM_SYSDEADCHAR);
	}
	/* restore shift key */
	rgchKeys[VK_SHIFT] = chShift;
	rgchKeys[VK_MENU] = chAlt;
	rgchKeys[VK_CONTROL] = chCtrl;
	SetKeyboardState(rgchKeys);
	if (bKey) {
	    /* have xlated key in msg struct */
	    switch(szKey[0] = LOBYTE(msg.wParam)) {
	    /* these are translated to undisplayable codes */
	    case ',':		/* virtkey is weird, we display comma */
		ch = ',';	/* as string */
	    case VK_TAB:	/* tab */
	    case VK_RETURN:	/* enter */
	    case VK_BACK:	/* backspace */
	    case VK_ESCAPE:	/* esc */
	    case ' ':
		bKey = FALSE;	/* make GetKeyString do it */
		break;
	    default:
		szKey[1] = '\0';
	    }
	}
	if (!bKey)
	    /* no virtkey translation (or can't use it) */
	    GetKeyString(ch,szKey);
#ifdef WIN2
	sprintf(szScratch,szKeyCmd,i,sz,szKey,szWin,
	  (LONG) lpt->time * 55L);
#else
	wsprintf(szScratch,szKeyCmd,i,(LPSTR) sz,(LPSTR) szKey,(LPSTR) szWin,
	  (LONG) lpt->time * 55L);
#endif
    }
    else
	/* mouse coordinate msg */
#ifdef WIN2
	sprintf(szScratch,szMouseCmd,i,sz,lpt->paramL,
	  lpt->paramH,szWin, (LONG) lpt->time * 55L);
#else
	wsprintf(szScratch,szMouseCmd,i,(LPSTR) sz,lpt->paramL,
	  lpt->paramH,(LPSTR) szWin, (LONG) lpt->time * 55L);
#endif
    return 0;
}

BOOL APIENTRY dlgPBError(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    register PMACRO pm;
    LPRECORDERMSG lpt, lptS;


    CHAR szTemp[CCHSCRATCH];
#ifndef WIN2
    CHAR *sz;
#endif
    INT rc;

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	/* set error message */
	LdStr(hDlg,pbe.ecode);
	SetDlgItemText(hDlg,IDEDIT,szScratch);

	/* center this sucker */
	CenterMe(hDlg);

	for (pm = pmFirst; pm; pm = pm->pmNext)
	    if (pm->id == pbe.id) {
		/* got it, do hot key and comment */
		HotKeyString(pm,TRUE);
#ifndef WIN2
		/* single-line EDIT can't deal with hotkey tab, so
		    space it out if there */
		if (sz = strchr(szScratch,'\t')) {
		    memmove(sz+8,sz + 1,strlen(sz));
		    memset(sz,' ',8);
		}
#endif
		SetDlgItemText(hDlg,IDEDIT2,szScratch);
		/* do code line */

		if ((rc = LoadMacro(pm)) == 0) {
		    lpt = (LPRECORDERMSG) GlobalLock(pm->hMemCode);

#ifdef NTPORT
		    lptS=lpt;
		    lpt =(LPRECORDERMSG)((LONG)lpt+(LONG)pbe.offset);

		    if ((rc = MsgString(lpt,lptS)) == 0)
			SetDlgItemText(hDlg,IDEDIT1,szScratch);

#else
		    lpt =(LPRECORDERMSG)((LONG)lpt|(LONG)pbe.offset);

		    if ((rc = MsgString(lpt)) == 0)
			SetDlgItemText(hDlg,IDEDIT1,szScratch);
#endif


		    if (pm->f.cxScreen != 0) {
			/* macro has mouse messages, screen res
			    differences may be relevent */
			if (pm->f.cxScreen != cxScreen || pm->f.cyScreen
			  != cyScreen) {
			    LdStr(hDlg,M_WRONGSCREEN);
#ifdef WIN2
			    sprintf(szTemp,szScratch,pm->f.cxScreen,
			      pm->f.cyScreen);
#else
			    wsprintf(szTemp,szScratch,pm->f.cxScreen,
			      pm->f.cyScreen);
#endif
			    SetDlgItemText(hDlg,IDTEXT,szTemp);
			}
		    }
		    GlobalUnlock(pm->hMemCode);
		}
		if (rc)
		    /* will get error msg on top of whatever we managed to
			get here */
		    ErrMsg(hDlg,rc);
		break;
	    }
/* things very messed up if not found */
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
#else
	switch (wParam) {
#endif
	case IDOK:
	case IDCANCEL:
	    EndDialog (hDlg,TRUE);
	    break;
	default:
	    return FALSE;
	}
	break;
    default:
	return FALSE;
    }
    return TRUE;
    lParam; /* -W3 */
}

static VOID NEAR CenterMe(HWND hWnd)
{
    RECT rect;

    GetWindowRect(hWnd,&rect);
    MoveWindow(hWnd,(cxScreen - (rect.right - rect.left))/2,
      (cyScreen - (rect.bottom - rect.top))/2,
      rect.right - rect.left,rect.bottom - rect.top,FALSE);
}
/* ------------------------------ EOF --------------------------------- */
