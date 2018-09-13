/****************************************************************************
NAME

    rinit.c -- discardable init segment for Recorder

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
    sds		3 Jan 89	Initial Coding
    sds		6 Jan		now checks TDyn version
    sds		10 Jan		now sets lpfnEnum
		11 Jan		ifdef'ed out ComboInit
		12 Jan		now support autoload
				added version check if using SDK combo's
		1 Feb 89	Now getting system font face and size
		1 March 89	no longer getting system font face and size
		13 Mar 89	restored Win2.1 support
		14 Mar 89	timer stuff
		16 Mar 89	added cmd line hot key support
		17 Mar		altered size-down method
    sds		25 Mar 1989	now loading ERRRESOURCE
    sds		28 Mar 1989	szTempDir
    sds		29 Mar 1989	cmd line improvements
    sds		30 Mar 1989	now loads E_NOMEM string
    sds		6 Apr 1989	TDYN version now distinct from file
    sds		27 Apr 1989	loads M_DESKINT
    sds		3 May 1989	(except if WIN2)
    sds		5 May 1989	now minimizes if have cmd line hot key
    sds		5 May 1989	removed szDeskInt load
    sds		9 May 1989	now creates Keybd xlate window
    sds		16 May 1989	setting bProtMode
    sds		18 May 1989	WIN2 change, REMOTECTL
    sds		22 May 1989	now passing cmd line to previous Tracer
    sds		24 May 1989	moved remote ctl code to REMOTE.C
    sds		25 May 1989	now passes cwd on cmd line to other Tracer
    sds		26 May 1989	fixed no-caption bug, WIN2 upgrade
    sds		9 Jun 1989	name change to Recorder, LB initially disabled
    sds		12 Jun 1989	bProtMode now bNotBanked, WIN2 changes
    sds		16 Jun 1989	fixed display problem loading listbox
				  at init time
    sds		27 Jul 1989	now using AnsiUpper, not toupper
				loading .hlp
				WIN.INI strings now hard coded
    sds		4 Aug 1989	now restores prev instance
    sds		25 Aug 1989	removed "include winexp.h", in windows.h now
    sds		18 Sep 1989	now using WinFlags to set bNotBanked
    sds		19 Sep 1989	removed unused error msg reference
    sds		20 Sep 1989	now using COLOR_WINDOW for bckgnd brush
    sds		20 Sep 1989	winexp.h restored for win2
    sds		28 Sep 1989	background brush now NULL
    sds		5 Oct 1989	listbox now NOINTEGRALHEIGHT
    sds		10 Oct 1989	cmdline from new instance now better about
				  saving existing data
    sds		31 Jan 1990	fixed benign but inelegant use of
    				  hWndRecorder while still NULL
    sds 	8 Feb 1990	Win2.1 compatibility
    sds 	15 Mar 1990	Win2.1 compatibility
    sds 	8 Jan 1991	fixed bug with "-h {hot key}" parsing
				MSC 6 fixups

*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */
#include <windows.h>
#include <port1632.h>
#ifdef WIN2
#include <winexp.h>
#endif
#include <string.h>
#include <stdio.h>
#include <io.h>
#include <dos.h>
#include <ctype.h>
#include <direct.h>
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */
#ifdef WIN2
#define LBS_USETABSTOPS 0
#define LBS_NOINTEGRALHEIGHT 0
#endif

/* FUNCTIONS DEFINED */
#ifndef WIN2
static VOID NEAR AbortMsg(HANDLE hInst);
#endif
static INT NEAR CmdLineHotKey(VOID);
static BOOL NEAR GetNextArg(CHAR FAR **,CHAR *);
static BOOL NEAR LoadStr(CHAR **,WORD);
static VOID NEAR PassTheBuck(LPSTR lpszCmdLine);
static BOOL NEAR Register(HANDLE hInst);

#ifndef NTPORT
INT APIENTRY WinMain(HANDLE, HANDLE, LPSTR, INT);
#endif

/* VARIABLES DEFINED */
static CHAR *szCmdLineHotKey=NULL;

/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */

#ifdef NTPORT
MMain(hInst,hPrevInst,lpCmdLine,nCmdShow)
#else
INT APIENTRY WinMain( HANDLE hInstance,
		      HANDLE hPrevInstance,
		      LPSTR  lpszCmdLine,
		      INT    cmdShow )
{
#endif
INT rc;

#ifdef NTPORT

LPSTR lpszCmdLine;			  // Temporary!!!
INT   cmdShow;				  // Temporary!!!
HANDLE hPrevInstance;			  // Temporary!!!
HANDLE hInstance;			  // Temporary!!!
					  // Temporary!!!
    lpszCmdLine=lpCmdLine;		  // Temporary!!!
    hInstance=hInst;			  // Temporary!!!
    hPrevInstance=hPrevInst;		  // Temporary!!!
    cmdShow=nCmdShow;			  // Temporary!!!

#endif

#ifndef WIN2
#ifdef NTPORT
    if (GETMAJORVERSION(GetVersion()) < 3) {
#else
    if (LOBYTE(GetVersion()) < 3) {
#endif
	AbortMsg(hInstance);
	return 0;
    }
#endif

#ifdef NTPORT
    if (!lpszCmdLine)		lpszCmdLine="\0";   // Temporary!!!
    if (cmdShow==SW_SHOWNORMAL) cmdShow=SW_SHOW;    // Temporary!!!
    hPrevInstance=0;				    // Temporary!!!
#endif

    if (hPrevInstance) {
	/* recordll can't handle two Recorders */
	PassTheBuck(lpszCmdLine);
	return 0;
    }

    if (!Register(hInstance))
	return FALSE;

    /* Make window visible according to the way the app is activated */
    ShowWindow(hWndRecorder, cmdShow);
    UpdateWindow(hWndRecorder);

    /* do command line AFTER painting window, else listbox gets confused */
    DoCmdLine(lpszCmdLine);

    if (GetActiveWindow() == hWndRecorder && pmFirst)
	SetFocus(hWndList);

    if (szCmdLineHotKey) {
	szErrArg = szCmdLineHotKey;
	if (szFileName == NULL)
	    rc = E_NOFILENOHOT;
	else
	    rc = CmdLineHotKey();
	if (rc)
	    ErrMsgArg(hWndRecorder,rc,szErrArg);
	FreeLocal(szCmdLineHotKey);
	szCmdLineHotKey = NULL;
    }

    return MainLoop();
}

static BOOL NEAR Register(HANDLE hInst)
/* initialization stuff broken out of WinMain above to not take any
    permanent stack space */
{
    WNDCLASS Class;
#ifndef WIN2
    INT rc;
#endif

    Class.style = CS_HREDRAW | CS_VREDRAW;
    Class.lpfnWndProc = WndProc;
    Class.cbClsExtra = Class.cbWndExtra = 0;
    hInstRecorder = Class.hInstance = hInst;
    Class.hIcon = LoadIcon( hInst, szRecorder);
    Class.hCursor = hCurNormal = LoadCursor( NULL, IDC_ARROW );
    Class.hbrBackground  = (HBRUSH) NULL;
    Class.lpszMenuName = (LPSTR) NULL;
    Class.lpszClassName = (LPSTR)szRecorder;

    if (!RegisterClass(&Class))
	return FALSE;

    /* create a bogus window used only as target for TranslateMessage
	call to xlate a virtkey WM_KEYDOWN to an ANSI WM_CHAR, for
	displaying macro list */

    Class.style = 0;
    Class.lpfnWndProc = KeyWndProc;
    Class.hbrBackground  = NULL;
    Class.lpszMenuName = (LPSTR) NULL;
    strcat(strcpy(szScratch,szRecorder),"KeyXlate");
    Class.lpszClassName = (LPSTR)szScratch;

    if (!RegisterClass(&Class))
	return FALSE;

    if ((hWndKey=CreateWindow( szScratch,
			      (LPSTR) "",
			       0L,
			       0, 0, 1, 1,
			      (HWND)NULL,
			      (HMENU)NULL,
			       hInst,
			      (LPSTR)NULL)) == 0)
	return FALSE;

    hMenuRecorder = LoadMenu(hInstRecorder,MAKEINTRESOURCE(RMENU));

    LoadStr(&szCaption, M_CAPTION);

#ifdef NTPORT
    if (CreateWindow( szRecorder,
		      szCaption,
		      WS_TILEDWINDOW,
		      CW_USEDEFAULT, 0, CW_USEDEFAULT, CW_USEDEFAULT,
		     (HWND)NULL,
		      hMenuRecorder,
		      hInst,
		     (LPSTR)NULL) == 0)
	return FALSE;
#else
    if (hWndRecorder=CreateWindow( szRecorder,
				   szCaption,
				   WS_TILEDWINDOW,
				   CW_USEDEFAULT, 0, CW_USEDEFAULT, CW_USEDEFAULT,
				  (HWND)NULL,
				   hMenuRecorder,
				   hInst,
				  (LPSTR)NULL) == 0) return FALSE;
#endif




    if ((hWndList=CreateWindow("ListBox",
			       (LPSTR)NULL,
				LBS_NOTIFY|LBS_USETABSTOPS|LBS_NOINTEGRALHEIGHT|
				WS_VISIBLE|WS_BORDER|WS_CHILD|WS_VSCROLL|
				WS_GROUP|WS_TABSTOP|WS_DISABLED,
				0, 0, 0, 0,
				hWndRecorder,
			       (HMENU)ID_LISTBOX,
				hInst,
				NULL)) == 0)
	return FALSE;

#ifndef WIN2
    rc = 28*4;
    SendMessage(hWndList,LB_SETTABSTOPS,1,(LONG) (INT FAR *)&rc);
#endif

    if (InitRDLL(hWndRecorder,bNotBanked) != TVERSION) {
	ErrMsg(hWndRecorder,E_BADTDYN);
	DestroyWindow(hWndRecorder);
	return FALSE;
    }
    return TRUE;
}

VOID DoCmdLine(LPSTR lpszCmdLine)
{
    CHAR chSwitch,chHotSwitch;
    CHAR szTemp[CCHSCRATCH];
    CHAR szFile[CCHSCRATCH];
    CHAR *pch;
    INT rc;

    /* deal with command line */
    LoadString(hInstRecorder,M_SWITCH,szScratch,CCHSCRATCH);
    chSwitch = szScratch[0];
    LoadString(hInstRecorder,M_HOTSWITCH,szScratch,CCHSCRATCH);
    chHotSwitch = szScratch[0];
    rc = 0;
    szFile[0] = '\0';
    while (GetNextArg(&lpszCmdLine,szTemp)) {
	szErrArg = szTemp;
	if (szTemp[0] == chSwitch) {
	    if ((UCHAR)((DWORD)AnsiUpper((LPSTR) MAKELONG(szTemp[1],0))) ==
	      (UCHAR)((DWORD)AnsiUpper((LPSTR) MAKELONG(chHotSwitch,0)))) {
		/* have a hot key */
		if (strlen(szTemp) > 2)
		    /* no separator between switch and hot key - tolerate */
		    pch = szTemp + 2;
		else {
		    if (!GetNextArg(&lpszCmdLine,szTemp))
			pch = NULL;
		    else
			pch = szTemp;
		}

		if (szCmdLineHotKey || pch == NULL) {
		    /* already have hot key, or -h EOL??? */
		    rc = E_BADCMDLINE;
		    break;
		}
		else {
		    if ((szCmdLineHotKey = AllocLocal(strlen(pch) + 1))
		      == 0) {
			rc = E_NOMEM;
			break;
		    }
		    else
			strcpy(szCmdLineHotKey,pch);
		}
	    }
	    else {
		/* bogus switch */
		rc = E_BADCMDLINE;
		break;
	    }
	}
	else {
	    /* looks like a file name */
	    if (szFile[0]) {
		/* already have a file */
		rc = E_BADCMDLINE;
		break;
	    }
	    strcpy(szFile,szTemp);
	}   /* end file name arg */
    }

    if (rc) {
	ErrMsgArg(hWndRecorder,rc,szErrArg);
	if (szCmdLineHotKey) {
	    /* already a mess - don't bitch about hot key too */
	    FreeLocal(szCmdLineHotKey);
	    szCmdLineHotKey = 0;
	}
	return;
    }

#ifdef NTPORT
    if (szFile[0] && SendMessage(hWndRecorder,WM_COMMAND,GET_WM_COMMAND_MPS(ID_NEW,0,0))) {
#else
    if (szFile[0] && SendMessage(hWndRecorder,WM_COMMAND,ID_NEW,0L)) {
#endif
	szErrArg = szFile;
	if (OpenIt(hWndRecorder,szFile))
	    DoCaption();
	LoadMacroList();
    }
}

static INT NEAR CmdLineHotKey()
/* play hot key from command line */
{
    CHAR *pch;
    CHAR szTemp[20];
    INT flags,i;
    UCHAR key;
    register struct macro *pm;

    flags = 0;
    key = '\0';

    pch = szCmdLineHotKey;
    do {
	switch(*pch) {
	case '^':
	    if (flags & B_CONTROL)
		return E_BOGUSHOTKEY;
	    flags |= B_CONTROL;
	    break;
	case '%':
	    if (flags & B_ALT)
		return E_BOGUSHOTKEY;
	    flags |= B_ALT;
	    break;
	case '+':
	    if (flags & B_SHIFT)
		return E_BOGUSHOTKEY;
	    flags |= B_SHIFT;
	    break;
	case '\0':
	    return E_BOGUSHOTKEY;
	case '{':
	    pch++;
	    i = 0;
	    do {
		if (i >= 20 || *pch == '\0')
		    return E_BOGUSHOTKEY;
		szTemp[i++] = *pch++;
	    } while (*pch != '}');
	    szTemp[i] = '\0';
	    if ((key = GetKeyCode(szTemp)) == NULL)
		return E_BOGUSHOTKEY;
	    break;
	default:
	    if ((key = GetKeyCode(pch)) == NULL)
		return E_BOGUSHOTKEY;
	}
	pch++;
    } while (key == 0);

    for (pm = pmFirst; pm; pm = pm->pmNext)
	if (pm->f.hotkey == key && (pm->f.flags & (B_SHFT_CONT_ALT))
	  == (WORD)((flags & (B_SHFT_CONT_ALT)))) {
	    /* got it - iconize to get Recorder out of the way */
	    ShowWindow(hWndRecorder,SW_MINIMIZE);
	    PlayStart(pm,FALSE);
	    break;
	}
    return (pm ? 0 : E_HKNOFILE);
}

static BOOL NEAR GetNextArg(CHAR FAR **plpsz,CHAR *psz)
/* read next arg off command line into psz, return FALSE if none */
{
    INT i;
    LPSTR lpch;
    CHAR *pch;
    BOOL bBrackets;

    lpch = *plpsz;

    /* skip spaces, tabs */
    while (*lpch != '\0' && isspace(*lpch))
	lpch++;

    if (*lpch == '\0')
	return FALSE;

    bBrackets = FALSE;

    for (i = 0, pch = psz; i < 80; i++) {
	switch (*pch++ = *lpch++) {
	case '{':
	    if (!bBrackets)
		/* entering {} hot key specification */
		bBrackets = TRUE;
	    break;
	case '}':
	    if (bBrackets)
		/* leaving {} hot key specification */
		bBrackets = FALSE;
	}
	if (*lpch == '\0')
	    /* end of cmd line */
	    break;
	if (!bBrackets && isspace(*lpch))
	    /* space between args */
	    break;
    }

    if (i >= 80) {
	ErrMsg(hWndRecorder,E_BADCMDLINE);
	return FALSE;
    }
    *pch = '\0';
    *plpsz = lpch;
    return TRUE;
}

VOID FAR CreateTime(HWND hWnd)
/* called from WM_CREATE to get initialized */
{
    INT i;

    hWndRecorder = hWnd;

#ifdef WIN2
    SBListRegister(hInstRecorder,"rec_list");
    SBComboRegister(hInstRecorder,"rec_combo","rec_list","edit");
#else
    /* are we banking EMS pages? */
    bNotBanked = !(MGetWinFlags() & (WF_LARGEFRAME|WF_SMALLFRAME));
#endif

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    hCurWait = LoadCursor(NULL,IDC_WAIT);

    DefFlags = (BYTE) GetProfileInt(szRecorder,szPlayback,
      B_PLAYTOWINDOW|B_WINRELATIVE|B_NESTS);
    DefMouseMode = (BYTE) GetProfileInt(szRecorder,szRecord,RM_DRAGS);

    for (i = 0; i < CMSGTEXT; i++)
	LoadStr(&rgszMsg[i],MSG_LDOWN + i);
    LoadStr(&szErrResource,E_ERRRESOURCE);
    LoadStr(&szMouseCmd,M_MOUSECMD);
    LoadStr(&szKeyCmd,M_KEYCMD);
    LoadStr(&szTempDir,M_TEMPDIR);
    LoadStr(&szTempFile,M_TEMPFILE);
    LoadStr(&szStarExt,M_STAREXT);
    LoadStr(&szDlgInt,M_DLGINT);
    LoadStr(&szDialogBox,M_DLGBOX);
#ifdef WIN2
    LoadStr(&szMsgInt,M_MSGINT);
    LoadStr(&szMessageBox,M_MSGBOX);
    LoadStr(&szMSDOS,M_MSDOS);
    LoadStr(&szWin200,M_WIN200);
#endif
    LoadStr(&szWindows,M_WINDOWS);
    LoadStr(&szDesktop,M_DESKTOP);
    LoadStr(&szWinOldApp,M_WINOLDAPP);
    LoadStr(&szControl,M_CONTROL);
    LoadStr(&szAlt,M_ALT);
    LoadStr(&szShift,M_SHIFT);
    LoadStr(&szScreen,M_SCREEN);
    LoadStr(&szWindow,M_WINDOW);
    LoadStr(&szNothing,M_NOTHING);
    LoadStr(&szEverything,M_EVERYTHING);
    LoadStr(&szClickDrag,M_CLICKDRAG);
    LoadStr(&szAnyWindow,M_ANYWINDOW);
    LoadStr(&szSameWindow,M_SAMEWINDOW);
    LoadStr(&szFast,M_FAST);
    LoadStr(&szRecSpeed,M_RECSPEED);
    LoadStr(&szUntitled,M_UNTITLED);
    LoadStr(&szNoMem,E_NOMEM);
    LoadStr(&szWarning,M_WARNING);
    LoadStr(&szError,M_ERROR);
    LoadStr(&szDotHlp,M_DOTHLP);

    lpfnEnum = MakeProcInstance(EnumProc,hInstRecorder);
    lpfnWndProc = MakeProcInstance((FARPROC) WndProc,hInstRecorder);

#ifdef REMOTECTL
    RemoteRegister();
#endif
    DoCaption();
}

static BOOL NEAR LoadStr(CHAR **psz, WORD id)
/* load string id, allocate space, stuff ptr in *psz */
{
    INT cch;

    LoadString(hInstRecorder,id,szScratch,CCHSCRATCH - 1);
    if ((*psz = AllocLocal(cch = strlen(szScratch) + 1)) == 0)
	return FALSE;
    memcpy(*psz,szScratch,cch);
}
#ifndef WIN2
static VOID NEAR AbortMsg(HANDLE hInst)
{
    CHAR sz[CCHSCRATCH];

    LoadString(hInst,M_CAPTION,sz,CCHSCRATCH - 1);
    LoadString(hInst,M_BADWINVERSION,szScratch,CCHSCRATCH - 1);
    MessageBox(NULL,szScratch,sz,MB_OK);
}
#endif

static VOID NEAR PassTheBuck(LPSTR lpszCmdLine)
{
    HWND hWnd;
    HANDLE hMem;
    INT cch;
    LPSTR lpsz;

    if (hWnd = FindWindow(szRecorder,NULL)) {

	if (IsIconic(hWnd))
	    ShowWindow(hWnd,SW_RESTORE);

#ifdef NTPORT
	SetForegroundWindow(hWnd);
#else
	SetActiveWindow(hWnd);
#endif
	if (lpszCmdLine && *lpszCmdLine) {
	    /* put directory on as first argument, else real Recorder may
		not find file */
	    if (_getcwd(szScratch,CCHSCRATCH)) {
		/* make sure it's upper case */
		AnsiUpper(szScratch);
		strcat(szScratch," ");
		cch = strlen(szScratch) + lstrlen(lpszCmdLine) + 1;
		if (hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,(LONG) cch)) {
		    lstrcpy(lpsz = GlobalLock(hMem),szScratch);
		    lstrcat(lpsz,lpszCmdLine);
		    GlobalUnlock(hMem);
#ifdef NTPORT
		    SendMessage(hWnd,WM_COMMAND,ID_CMDLINE,(LONG)hMem);
#else
		    SendMessage(hWnd,WM_COMMAND,ID_CMDLINE,MAKELONG(hMem,0));
#endif
		    GlobalFree(hMem);
		}
	    }
	}
    }
}
/* ------------------------------ EOF --------------------------------- */
