/**************************************************************************
NAME

    fileio.c -- Recorder file I/O

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
		9 Dec		Now use AddMacro for correct ordering
		14 Dec		fixed bug leaving old macros around
				when opening new file
		15 Dec		version 2.  macro flags byte now a word
		16 Dec		fixed bug in dbl click on blank listbox
				 row
		17 Dec		version 3. cMsgs now in paramL of 1st msg
				now using PMACRO
				macro now in EMS memory
		20 Dec		LoadIt now sets message field in macro
				to current flags word, in case it was edited.
		30 Dec		Version 4 - with screen size in macro
		3 Jan 89	More sensible control id's
		6 Jan		moved VERSION to tdyn.h
		26 Jan 89	improved DOOKBUTTON
		31 Jan 89	improved filename input handling
		2 Feb 89	now deals with font name string in macro hdr
		10 Feb 89	another DoOkButton improvement
		1 Mar 89	removed font support
		15 Mar 89	fixed file merge bug
		16 Mar 89	improved error reporting
		17 Mar 89	now uppercasing input file name 
		28 Mar 1989	output file now written to TEMP or \
				of destination drive before rename
		29 Mar 1989	now warns of overwrite existing file
		30 Mar 1989	LdStr upgrade
		31 Mar 1989	fixed check-for-existing-file bug, better
				bogus filename handling
		3 Apr 1989	dlgSave now hilites EDIT ctl string
		26 Apr 1989	dlgSave now uppercases new file name
		2 May 1989	fixed bug in dodirlb
		16 May 1989	tdyn.h now tracerd.h
				healed drain-bamaged dlgSave
		18 May 1989	now using OpenIt to load file
		18 May 1989	massive user intfc changes
		23 May 1989	fixed bug losing merged macro code/desc
		26 May 1989	added directory listbox to File/Save
		30 May 1989	fixed initial dir in File/Save
    sds		30 May 1989	system menu support
    sds		7 Jun 1989	removed unused code
    sds		9 Jun 1989	name change to recorder
    sds		12 Jun 1989	SaveIt now supports bOverWrite
    sds		3 Jul 1989	FixSysMenu not needed for Win30
				now using wsprintf
    				merge not sets bFileChanged
    sds		27 Jul 1989	now using AnsiUpper, not toupper
    sds		18 Aug 1989	AddHotKey now passes id
				fixed bug not reporting File/Save errors
				fixed File/Save to bogus file handling
    sds		29 Sep 1989	fixed oem/ansi bugs with file names
    sds		10 Oct 1989	fixed bug not restoring listbox on
				  failed file/merge
				workaround for OpenFile oddity of looking
				  in Windows startup directory
    sds		11 Oct 1989	now catches _dos_close error
    sds		31 Oct 1989	dlgSave no longer puts template in EDIT ctl
				now enables OK only if file name not blank
    sds		28 Nov 1989	E_NOTRECORDERFILE on read of bogus file
    sds		30 Nov 1989	changed MB_ICONQUESTION to MB_ICONEXCLAMATION
    sds		26 Dec 1989	warns on merge creating 2 macros with no
				  hotkey but identical names
    sds		19 Jan 1990	wait cursor on File/Save
    sds		29 Jan 1990	removed OpenFile/Windows root directory
    				  workaround.  OpenFile is fixed
				minor fix on wait cursor in File/Save
*/
/* -------------------------------------------------------------------- */
/*	Copyright (c) 1990 Softbridge, Ltd.				*/
/* -------------------------------------------------------------------- */

/* INCLUDE FILES */

#include <windows.h>
#include <port1632.h>
#include <string.h>
#include <stdio.h>
#undef max
#undef min
#include <stdlib.h>
#include <io.h>

#ifndef NTPORT
#include <dos.h>
#endif

#include <direct.h>
#include <ctype.h>
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */
#define MAGIC_HDR   0x1776	/* history quiz - what are these numbers? */
#define MAGIC_CODE  0x1789
#define MAGIC_MACRO 0x1969
#define MAGIC_DESC  0x1492

/* the very first bytes of file */
struct header {
    WORD magic;
    WORD version;
    LONG lPosMacros;
    WORD cMacros;
};
/* the remainder of file is as follows:
  cMacros instances of
    lPosCode-> MAGIC_CODE
		code itself (suitable for putting in global segment
			     and handing to tdyn for playback)
    lPosDesc-> MAGIC_DESC
		description
  lPosMacros-> MAGIC_MACRO
    cMacros instances of
      struct filemacro
*/

/* FUNCTIONS DEFINED */
static VOID NEAR DoDirLB(HWND,LONG);

#ifdef NTPORT
static VOID NEAR DoFileLB(HWND,WORD);
#else
static VOID NEAR DoFileLB(HWND,LONG);
#endif

static BOOL NEAR DoOKButton(HWND,CHAR *);
static INT NEAR LoadIt(PMACRO,BOOL);
static BOOL NEAR NewFileSpec(HWND,CHAR *);
static VOID NEAR SetEdit(HWND,CHAR *);

/* VARIABLES DEFINED */
static CHAR *szFileSpec=0;	/* current file template */
static CHAR szDirTmplt[]="__q__qxz.^^?";    /* DlgDirList needs something
				    ridiculous that will never match */
static CHAR szSlash[2] = {CHDIRSEPARATOR,0};

/* FUNCTIONS REFERENCED */

/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */
static VOID NEAR SetEdit( HWND hDlg, CHAR *sz)
/* set text in IDEDIT control and select whole string */
{
    SetDlgItemText(hDlg,IDEDIT,sz);

#ifdef NTPORT
    SendMessage(GetDlgItem(hDlg,IDEDIT),EM_SETSEL,GET_EM_SETSEL_MPS(0,32767));
#else
    SendDlgItemMessage(hDlg,IDEDIT,EM_SETSEL,0,MAKELONG(0,32767));
#endif
}

#ifdef NTPORT
static VOID NEAR DoFileLB(HWND hDlg,WORD wCmd)
/* handle WM_COMMAND for file listbox of file/dir pair */
{
    HWND hWnd;
    INT rc;

    switch(wCmd) {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
	/* set edit box to selected file name */
	hWnd = GetDlgItem(hDlg,IDLIST);
	rc = (WORD) SendMessage(hWnd,LB_GETCURSEL,0,0L);
	if (rc >= 0) {
	    SendMessage(hWnd,LB_GETTEXT,rc,(LONG) (LPSTR) szScratch);
	    SetEdit(hDlg,szScratch);
	    if (wCmd == LBN_DBLCLK)
	    SendMessage(hDlg,WM_COMMAND,GET_WM_COMMAND_MPS(IDOK,0,0));
	}
    }
}
#else
static VOID NEAR DoFileLB(HWND hDlg,LONG lParam)
/* handle WM_COMMAND for file listbox of file/dir pair */
{
    HWND hWnd;
    INT rc;

    switch(HIWORD(lParam)) {
    case LBN_SELCHANGE:
    case LBN_DBLCLK:
	/* set edit box to selected file name */
	hWnd = GetDlgItem(hDlg,IDLIST);
	rc = (WORD) SendMessage(hWnd,LB_GETCURSEL,0,0L);
	if (rc >= 0) {
	    SendMessage(hWnd,LB_GETTEXT,rc,(LONG) (LPSTR) szScratch);
	    SetEdit(hDlg,szScratch);
	    if (HIWORD(lParam) == LBN_DBLCLK)
		SendMessage(hDlg,WM_COMMAND,IDOK,0L);
	}
    }
}
#endif

static VOID NEAR DoDirLB(HWND hDlg,LONG lParam)
/* handle WM_COMMAND for directory listbox of file/dir pair */
{
    /* get directory in usable form */

#ifdef NTPORT
    MDlgDirSelect(hDlg, szScratch, CCHSCRATCH, IDDIRLIST);
#else
    DlgDirSelectEx(hDlg,szScratch,IDDIRLIST);
#endif

    switch(HIWORD(lParam)) {
    case LBN_SELCHANGE:
	/* update EDIT box with dir\template */
	strcat(szScratch,szFileSpec);
	SetEdit(hDlg,szScratch);
	break;
    case LBN_DBLCLK:
	/* update directory box with impossible file template */
	strcat(szScratch,szDirTmplt);
	DlgDirList(hDlg,szScratch,IDDIRLIST,IDTEXT,0x4010);

	/* update file list AFTER directory set */
	DlgDirList(hDlg,szFileSpec,IDLIST,0,0);
	/* update edit box */
	SetEdit(hDlg,szFileSpec);
    }
}

static BOOL NEAR DoOKButton(HWND hDlg,CHAR *pszFile)
/* handle OK button, return TRUE if really a file name */
{
    INT cch;
    CHAR *sz;
    CHAR chLast;

    cch = GetDlgItemText (hDlg,IDEDIT,pszFile,32);
    chLast = *(pszFile + cch - 1);
    if (HasWildCard(pszFile) || chLast == CHDIRSEPARATOR ||
      chLast == CHDRIVESEPARATOR) {
	/* wants a new template */
	if (strrchr(pszFile,CHDIRSEPARATOR) ||
	  strrchr(pszFile,CHDRIVESEPARATOR)) {
	    /* set directory first */
	    strcpy(szScratch,pszFile);
	    if (sz = strrchr(szScratch,CHDIRSEPARATOR))
		/* add template to end of path */
		strcpy(sz + 1,szDirTmplt);
	    else
		/* no path, add to drive */
		strcpy(strrchr(szScratch,CHDRIVESEPARATOR) + 1,szDirTmplt);
	    if (DlgDirList(hDlg,szScratch,IDDIRLIST,IDTEXT,0x4010)) {
		/* good directory */
		strcpy(szScratch,szFileSpec);
		if ((sz = strrchr(pszFile,CHDIRSEPARATOR)) == 0)
		    sz = strrchr(pszFile,CHDRIVESEPARATOR);
		if (*(sz + 1))
		    /* pull new file spec off end */
		    strcpy(szScratch,sz + 1);
		if (!NewFileSpec(hDlg,szScratch))
		    return FALSE;
		SetEdit(hDlg,szFileSpec);
		DlgDirList(hDlg,szFileSpec,IDLIST,0,0);
	    }
	    else
		/* else bogus directory */
		MessageBeep(0);
	    return FALSE;
	}
	else {
	    /* just a new template */
	    if (!NewFileSpec(hDlg,pszFile))
		return FALSE;
	    SetEdit(hDlg,szFileSpec);
	    DlgDirList(hDlg,szFileSpec,IDLIST,0,0);
	}
	return FALSE;
    }
    else {
	/* no wild cards, but may be a directory */
	strcat(strcat(pszFile,szDirSeparator),szFileSpec);
	if (DlgDirList(hDlg,pszFile,IDLIST,0,0)) {
	    /* this is a directory */
	    SetEdit(hDlg,szFileSpec);

	    /* update dir listbox, just need template.  Dir set above */
	    DlgDirList(hDlg,szDirTmplt,IDDIRLIST,IDTEXT,0x4010);
	    return FALSE;
	}
	/* was really a file, get rid of filespec ending */
	*(pszFile + cch) = '\0';
    }
    return TRUE;
}

BOOL APIENTRY dlgOpen(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    CHAR szFile[CCHSCRATCH];

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	/* file listbox gets normal template */
	if (!NewFileSpec(hDlg,szStarExt)) {
	    EndDialog(hDlg,FALSE);
	    break;
	}
	DlgDirList(hDlg,szFileSpec,IDLIST,NULL,0);

	/* directory listbox gets impossible template, but with dirs */
	DlgDirList(hDlg,szDirTmplt,IDDIRLIST,IDTEXT,0x4010);

	/* edit box gets current file spec */
	SetEdit(hDlg,szFileSpec);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
	case IDLIST:
	    DoFileLB(hDlg,GET_WM_COMMAND_CMD(wParam,lParam));
	    break;
	case IDDIRLIST:
	    DoDirLB(hDlg,GET_WM_COMMAND_CMD(wParam,lParam));
	    break;
	case IDEDIT:
	    if (GET_WM_COMMAND_CMD(wParam,lParam)== EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
#else
	switch (wParam) {
	case IDLIST:
	    DoFileLB(hDlg,lParam);
	    break;
	case IDDIRLIST:
	    DoDirLB(hDlg,lParam);
	    break;
	case IDEDIT:
	    if (HIWORD(lParam) == EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
#endif
	case IDOK:
	    if (!DoOKButton(hDlg,szFile))
		/* still messing with directories */
		break;
	    if (!OpenIt(hDlg,szFile))
		/* leave dlg box up */
		return FALSE;
	    LoadMacroList();
	    /* FALL THRU !!!! */
	case IDCANCEL:
#ifdef NTPORT
	    EndDialog (hDlg,GET_WM_COMMAND_ID(wParam,lParam)==IDOK);
#else
	    EndDialog (hDlg,wParam==IDOK);
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
}

BOOL OpenIt(HWND hWnd,LPSTR lpszFile)
/* pass file name as entered by user (ANSI).  return TRUE if it worked */
{
    register PMACRO pm;
    CHAR *sz;
    INT rc;
    OFSTRUCT of;
    CHAR szFile[CCHSCRATCH];

    /* go for it */
    SetCursor(hCurWait);
    /* discard old file and macros */
    FreeFile();

    lstrcpy(szFile,lpszFile);

    if ((sz = strrchr(szFile,CHDIRSEPARATOR)) == 0)
	sz = szFile;
    if (!strrchr(sz,CHFILEEXTSEPARATOR))
	/* add extension */
	strcat(sz,szStarExt + 1);

    szErrArg = of.szPathName;
    rc = RecReadFile(szFile,&of);
    SetCursor(hCurNormal);
    if (rc) {
	if (hFileIn != -1) {
	    _lclose(hFileIn);
	    hFileIn = -1;
	}
    }
    else {
	if ((szFileName = AllocLocal(strlen(of.szPathName) + 1)) == 0) {
	    FreeMacroList();
	    rc = E_NOMEM;
	}
	else {
	    /* set file name in ANSI */
	    OemToAnsi(of.szPathName,szFileName);
	    memcpy(&ofCurrent,&of,sizeof(OFSTRUCT));
	    for (pm = pmFirst; pm; pm = pm->pmNext)
		if (pm->f.hotkey)
		    if (!AddHotKey(pm->f.hotkey,pm->f.flags,pm->id)) {
			rc = E_TOOMANYHOT;
			break;
		    }
	}
    }
    if (rc) {
#ifdef REMOTECTL
	ForceRecActivate(TRUE);
#endif
	ErrMsgArg(hWnd,rc,szErrArg);
#ifdef REMOTECTL
	ForceRecActivate(FALSE);
#endif
    }
    return rc == 0;
}

static BOOL NEAR NewFileSpec( HWND hWnd,CHAR *sz)
/* hWnd:parent of error msg if needed */
/* sz:new file spec template */
{
    if (szFileSpec)
	FreeLocal(szFileSpec);
    if ((szFileSpec = AllocLocal(strlen(sz) + 1)) == 0) {
	ErrMsg(hWnd,E_NOMEM);
	return FALSE;
    }
    strcpy(szFileSpec,sz);
    return TRUE;
}

BOOL APIENTRY dlgMerge(HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    register PMACRO pm;
    PMACRO pmTemp;
    INT rc;
    CHAR szFile[CCHSCRATCH];
    CHAR *sz;
    OFSTRUCT of,ofTemp;
    BOOL bDupHotkey,bDupComment;

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	/* file listbox gets normal template */
	if (!NewFileSpec(hDlg,szStarExt)) {
	    EndDialog(hDlg,FALSE);
	    break;
	}
	DlgDirList(hDlg,szFileSpec,IDLIST,NULL,0);

	/* directory listbox gets impossible template, but with dirs */
	DlgDirList(hDlg,szDirTmplt,IDDIRLIST,IDTEXT,0x4010);

	/* edit box gets current file spec */
	SetEdit(hDlg,szFileSpec);
	break;
    case WM_COMMAND:
#ifdef NTPORT
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
	case IDLIST:
	    DoFileLB(hDlg,GET_WM_COMMAND_CMD(wParam,lParam));
	    break;
	case IDDIRLIST:
	    DoDirLB(hDlg,GET_WM_COMMAND_CMD(wParam,lParam));
	    break;
	case IDEDIT:
	    if (GET_WM_COMMAND_CMD(wParam,lParam)==EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
#else
	switch (wParam) {
	case IDLIST:
	    DoFileLB(hDlg,lParam);
	    break;
	case IDDIRLIST:
	    DoDirLB(hDlg,lParam);
	    break;
	case IDEDIT:
	    if (HIWORD(lParam) == EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
#endif
	case IDOK:
	    if (!DoOKButton(hDlg,szFile))
		/* still messing with directories */
		break;

	    SetCursor(hCurWait);
	    if ((sz = strrchr(szFile,CHDIRSEPARATOR)) == 0)
		sz = szFile;
	    if (!strrchr(sz,CHFILEEXTSEPARATOR))
		/* add extension */
		strcat(sz,szStarExt + 1);

	    /* let RecReadFile think it is loading new file */
	    pmTemp = pmFirst;
	    pmFirst = 0;
	    szErrArg = of.szPathName;
	    rc = RecReadFile(szFile,&of);
	    if (rc) {
		if (hFileIn != -1) {
		    _lclose(hFileIn);
		    hFileIn = -1;
		}
	    }
	    else {
		/* we only keep track of main file - must load code
		   and desc for each merged macro */
		memcpy(&ofTemp,&ofCurrent,sizeof(OFSTRUCT));
		memcpy(&ofCurrent,&of,sizeof(OFSTRUCT));
		for (pm = pmFirst; pm && rc == 0; pm = pm->pmNext) {
		    if ((rc = LoadMacro(pm)) == 0)
		        rc = LoadDesc(pm);
		    /* mark code/desc as unrecoverable/undiscardable */
		    pm->f.lPosCode = pm->f.lPosDesc = 0L;
		}
		if (rc == 0)
		    memcpy(&ofCurrent,&ofTemp,sizeof(OFSTRUCT));
	    }
	    SetCursor(hCurNormal);
	    if (rc) {
		/* forget it */
		FreeMacroList();
		pmFirst = pmTemp;
		LoadMacroList();
		ErrMsgArg(hDlg,rc,szErrArg);
	    }
	    else {
		/* add new macros and hot keys */
		pm = pmFirst;
		pmFirst = pmTemp;
		pmTemp = pm;

		bDupHotkey = bDupComment = FALSE;
		while (pm = pmTemp) {
		    if (pm->f.hotkey) {
			if (!CheckHotKey(pm->f.hotkey,pm->f.flags,NULL)) {
			    /* duplicate hot key - just clear it */
			    pm->f.hotkey = 0;
			    pm->f.flags &= ~(B_SHFT_CONT_ALT);
			    if (!bDupHotkey) {
				WarnMsg(hDlg,M_CLEAREDDUPHOT);
				bDupHotkey = TRUE;
			    }
			    if (pm->f.szComment[0] == '\0') {
				/* mark the clear with a comment */
				LdStr(hDlg,M_MERGECLEARED);
				strcpy(pm->f.szComment,szScratch);
			    }
			}
		    }
		    else {
			if (!bDupComment &&
			  !CheckComment(pm,pm->f.szComment)) {
			    WarnMsg(hDlg,M_DUPCOMMENT);
			    bDupComment = TRUE;
			}
		    }

		    /* take off newly loaded list */
		    pmTemp = pm->pmNext;
		    pm->pmNext = NULL;

		    /* add to old (real) list */
		    AddMacro(pm);

		    /* add hot key AFTER AddMacro sets id */
		    if (pm->f.hotkey && rc == 0)
			if (!AddHotKey(pm->f.hotkey,pm->f.flags,pm->id)) {
			    ErrMsg(hDlg,E_TOOMANYHOT);
			    rc = 1;
			}
		}
		LoadMacroList();
		bFileChanged = TRUE;
		rc = 0;
	    }
	    EndDialog (hDlg,rc == 0);
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
}

BOOL APIENTRY dlgSave( HWND hDlg, WORD msg, WPARAM wParam, LONG lParam)
{
    CHAR szTemp[CCHSCRATCH];
#ifdef DIRTARGET
    CHAR *sz;
#endif

    switch(msg) {
    case WM_INITDIALOG:
#ifdef WIN2
	FixSysMenu(hDlg);
#endif
	SetDlgItemText(hDlg,IDEDIT,
	  szFileName ? (LPSTR) szFileName : (LPSTR) szNull );

#ifdef DIRTARGET
/* compiled out so file name's directory is ignored, fill listbox
    with current directory.  Seems wrong, but that's the standard?? */
	if (szFileName) {
	    /* extract dir/path */
	    strcpy(szScratch,szFileName);
	    if ((sz = strrchr(szScratch,CHDIRSEPARATOR)) ||
	      (sz = strchr(szScratch,CHDRIVESEPARATOR)))
		*(sz + 1) = '\0';
	    else
		szScratch[0] = '\0';
	}
	else {
	    getcwd(szScratch,CCHSCRATCH - 1);
	    szScratch[rc = strlen(szScratch)] = CHDIRSEPARATOR;
	    szScratch[rc + 1] = '\0';
	}
	strcat(szScratch,szDirTmplt);
	DlgDirList(hDlg,szScratch,IDDIRLIST,IDTEXT,0x4010);
#endif
	DlgDirList(hDlg,szDirTmplt,IDDIRLIST,IDTEXT,0x4010);

#ifdef NTPORT
	  SendMessage(GetDlgItem(hDlg,IDEDIT),EM_SETSEL,GET_EM_SETSEL_MPS(0,32767));
#else
	SendDlgItemMessage(hDlg,IDEDIT,EM_SETSEL,0,MAKELONG(0,32767));
#endif
	break;

#ifdef NTPORT
    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wParam,lParam)) {
	case IDEDIT:
	    if (GET_WM_COMMAND_CMD(wParam,lParam)==EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
	case IDDIRLIST:
	    if (GET_WM_COMMAND_CMD(wParam,lParam)==LBN_DBLCLK) {
		MDlgDirSelect(hDlg, szScratch, CCHSCRATCH, IDDIRLIST);

		/* update directory box with impossible file template */
		strcat(szScratch,szDirTmplt);
		DlgDirList(hDlg,szScratch,IDDIRLIST,IDTEXT,0x4010);
	    }
	    break;
#else
    case WM_COMMAND:
	switch (wParam)	{
	case IDEDIT:
	    if (HIWORD(lParam) == EN_CHANGE) {
		EnableWindow(GetDlgItem(hDlg,IDOK),(BOOL)
		  GetDlgItemText(hDlg,IDEDIT,szScratch,CCHSCRATCH) > 0);
	    }
	    break;
	case IDDIRLIST:
	    if (HIWORD(lParam) == LBN_DBLCLK) {
		DlgDirSelectEx(hDlg, szScratch, IDDIRLIST);

		/* update directory box with impossible file template */
		strcat(szScratch,szDirTmplt);
		DlgDirList(hDlg,szScratch,IDDIRLIST,IDTEXT,0x4010);
	    }
	    break;
#endif
	case IDOK:
	    GetDlgItemText(hDlg,IDEDIT,szTemp,CCHSCRATCH);
	    if (SaveIt(hDlg,szTemp,FALSE))
		EndDialog (hDlg,TRUE);
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
}

BOOL SaveIt(HWND hWnd,LPSTR lpszFile,BOOL bOverWrite)
/* lpszFile is in ANSI */
{
    CHAR *sz;
    INT rc;
    OFSTRUCT of;
    CHAR szFile[CCHSCRATCH];
    CHAR szT[CCHSCRATCH];
    CHAR szAnsi[CCHSCRATCH];

    lstrcpy(szFile,lpszFile);
    if ((sz = strrchr(szFile,CHDIRSEPARATOR)) == 0)
	/* skip over leading path (and misleading .'s) */
	sz = szFile;
    if (!strrchr(sz,CHFILEEXTSEPARATOR))
	/* add extension */
	strcat(sz,szStarExt + 1);
    AnsiUpper(szFile);
    szErrArg = szFile;
    rc = 0;
    SetCursor(hCurWait);

#ifdef NTPORT
    if (OpenFile(szFile, &of, OF_PARSE) == -1)
	rc = E_BADFILENAME;
#else
    if (OpenFile (szFile, &of, OF_PARSE) == -1)
	rc = E_BADFILENAME;
#endif

    else {
	/* need ANSI copy of file name */
	OemToAnsi(of.szPathName,szAnsi);

	if (!bOverWrite && OpenFile(szFile,&of,OF_EXIST|OF_REOPEN) > 0) {
	    /* file already exists */
	    SetCursor(hCurNormal);
	    LoadString(hInstRecorder,M_REPLACEFILE,szT,CCHSCRATCH-1);
#ifdef WIN2
	    sprintf(szScratch, szT, szAnsi);
#else
	    wsprintf(szScratch, szT, (LPSTR)szAnsi);
#endif
	    if (MessageBox (hWnd, szScratch, szCaption, 
	      MB_YESNOCANCEL|MB_ICONEXCLAMATION|MB_DEFBUTTON2) != IDYES)
		/* leave it alone */
		return FALSE;
	    SetCursor(hCurWait);
	}
	szErrArg = szAnsi;

	rc = RecWriteFile(szAnsi);
	if (rc) {
	    if (hFileOut != -1) {
		_lclose(hFileOut);
		hFileOut = -1;
	    }
	}
	else {
	    /* successful save always sets file name */
	    if (szFileName)
		FreeLocal(szFileName);
	    if ((szFileName = AllocLocal(strlen(szAnsi) + 1)) == 0) {
		FreeMacroList();
		rc = E_NOMEM;
	    }
	    else
		/* set filename in ANSI */
		strcpy(szFileName,szAnsi);
	    if (rc == 0)
		WriteDone();
	}
    }
    SetCursor(hCurNormal);
    if (rc)
	ErrMsgArg(hWnd,rc,szErrArg);
    return rc == 0;
}

INT RecWriteFile(CHAR *szName)
/* write output file (name in ANSI).  ERROR return codes MUST take
   argument!!! */
{
    struct header hdr;
    struct filemacro fm;
    register PMACRO pm;
    INT cMacros;
    CHAR szOut[CCHSCRATCH];	/* output file name, with uniqueness */
    LPSTR lpCode;
    CHAR *sz;
    LONG lPos;
    WORD w;
    WORD rc;
    OFSTRUCT of;

    if (OpenFile(szName,&of,OF_PARSE) < 0)
	/* ill-formed file name? */
	return E_BADFILENAME;

    if ((sz = getenv(szTempDir)) != 0 && (sz[1] != CHDRIVESEPARATOR ||
      (sz[1] == CHDRIVESEPARATOR &&
      (UCHAR)((DWORD)AnsiUpper((LPSTR) MAKELONG(sz[0],0))) ==
      (UCHAR)((DWORD)AnsiUpper((LPSTR) MAKELONG(of.szPathName[0],0)))))) {

	/* TEMP is defined, it is on same drive, so use it */
	strcpy(szOut,sz);
	if (szOut[strlen(szOut) - 1] != CHDIRSEPARATOR)
	    strcat(szOut,szSlash);
    }
    else {
	/* else just use root of target drive */
	if (of.szPathName[1] == CHDRIVESEPARATOR) {
	    memcpy(szOut,of.szPathName,2);
	    strcpy(&szOut[2],szSlash);
	}
	else
	    strcpy(szOut,szSlash);
    }

    /* make a unique name for output file, so can just rename later */
    strcat(szOut,szTempFile);
    mktemp(szOut);

#ifdef NTPORT
    if ((hFileOut=M_lcreat(szOut,0))==-1) return E_CANTCREATEFILE;
#else
    if (_dos_creat(szOut,0,(INT *) &hFileOut)) {
	hFileOut = -1;
	return E_CANTCREATEFILE;
    }
#endif

    hdr.magic = MAGIC_HDR;
    hdr.version = FVERSION;

#ifdef NTPORT
    if ((rc=_lwrite(hFileOut,(LPSTR)&hdr,sizeof(struct header)))==-1 ||
	rc != sizeof(struct header)) return E_WRITE;
#else
    if (_dos_write(hFileOut,&hdr,sizeof(struct header),&rc) != 0 ||
      rc != sizeof(struct header))
	return E_WRITE;
#endif

    lPos = sizeof(struct header);

    /* write code itself */
    for (pm = pmFirst, cMacros = 0; pm; pm = pm->pmNext, cMacros++) {
	pm->lPosCodeT = lPos;
	w = MAGIC_CODE;

#ifdef NTPORT
	if ((rc=_lwrite(hFileOut,(LPSTR)&w,sizeof(WORD)))==-1 ||
	     rc != sizeof(WORD)) return E_WRITE;
#else
	if (_dos_write(hFileOut,&w,sizeof(WORD),&rc) != 0
	  || rc != sizeof(WORD))
	    return E_WRITE;
#endif

	lPos += sizeof(WORD);

	if (rc = LoadMacro(pm))
	    return rc;
	lpCode = GlobalLock(pm->hMemCode);

#ifdef NTPORT
	w = _lwrite(hFileOut,lpCode,pm->f.cchCode);
#else
	w = lwrite(hFileOut,lpCode,pm->f.cchCode);
#endif
	GlobalUnlock(pm->hMemCode);
	if (w != pm->f.cchCode)
	    return E_WRITE;
	lPos += pm->f.cchCode;

	if (rc = LoadDesc(pm))
	    return rc;
	if (pm->hMemDesc) {
	    pm->lPosDescT = lPos;
	    w = MAGIC_DESC;

#ifdef NTPORT
	    if ((rc=_lwrite(hFileOut,(LPSTR)&w,sizeof(WORD)))==-1 ||
		 rc != sizeof(WORD)) return E_WRITE;
#else
	    if (_dos_write(hFileOut,&w,sizeof(WORD),&rc) != 0
	      || rc != sizeof(WORD))
		return E_WRITE;
#endif
	    lPos += sizeof(WORD);

	    lpCode = GlobalLock(pm->hMemDesc);
#ifdef NTPORT
	w = _lwrite(hFileOut,lpCode,pm->f.cchDesc);
#else
	w = lwrite(hFileOut,lpCode,pm->f.cchDesc);
#endif
	    GlobalUnlock(pm->hMemDesc);
	    if (w != pm->f.cchDesc)
		return E_WRITE;
	    lPos += pm->f.cchDesc;
	}
    }

    /* write macros */
    w = MAGIC_MACRO;

#ifdef NTPORT
    if ((rc=_lwrite(hFileOut,(LPSTR)&w,sizeof(WORD)))==-1 ||
	 rc != sizeof(WORD)) return E_WRITE;
#else
    if (_dos_write(hFileOut,&w,sizeof(WORD),&rc) != 0
      || rc != sizeof(WORD))
	return E_WRITE;
#endif

    hdr.lPosMacros = lPos;
    hdr.cMacros = cMacros;
    for (pm = pmFirst; pm; pm = pm->pmNext) {
	memcpy(&fm,&pm->f,sizeof(struct filemacro));
	fm.lPosCode = pm->lPosCodeT;
	fm.lPosDesc = pm->lPosDescT;

#ifdef NTPORT
	if ((rc=_lwrite(hFileOut,(LPSTR)&fm,sizeof(struct filemacro)))==-1 ||
	     rc != sizeof(struct filemacro)) return E_WRITE;
#else
	if (_dos_write(hFileOut,&fm,sizeof(struct filemacro),&rc) != 0
	  || rc != sizeof(struct filemacro))
	    return E_WRITE;
#endif

    }

#ifdef NTPORT
    _llseek(hFileOut,0L,0);

    if ((rc=_lwrite(hFileOut,(LPSTR)&hdr,sizeof(struct header)))==-1 ||
	 rc != sizeof(struct header)) return E_WRITE;
#else
    _lseek(hFileOut,0L,SEEK_SET);

    if (_dos_write(hFileOut,&hdr,sizeof(struct header),&rc) != 0
      || rc != sizeof(struct header))
	return E_WRITE;
#endif

    rc = _lclose(hFileOut);
    hFileOut = -1;
    if (rc)
	return E_WRITE;

    /* everybody happy, delete old output file (if any), and rename */
    _unlink(of.szPathName);
    if (rename(szOut,of.szPathName)) {
	/* usually can't rename because its a bogus directory path, which
	    OpenFile does not catch - just blow away temp output file.
	    VERY unlikely that there actually was a preexisting file that
	    got blown away in this procedure */
	_unlink(szOut);
	return E_WRITE;
    }
    return 0;
}

VOID WriteDone()
/* call after a successful RecWriteFile */
{
    register PMACRO pm;

    for (pm = pmFirst; pm; pm = pm->pmNext) {
	/* update ptrs into file */
	pm->f.lPosCode = pm->lPosCodeT;
	pm->f.lPosDesc = pm->lPosDescT;
	/* don't need description in memory any more */
	ReleaseDesc(pm);
	/* ditto for code */
	ReleaseCode(pm);
    }
    bFileChanged = FALSE;
    /* make sure ofCurrent is up to date */
    OpenFile(szFileName,&ofCurrent,OF_PARSE);
}

INT LoadMacro(PMACRO pm)
/* load macro and return 0 or error */
{
    INT rc;

    if (pm->hMemCode)
	/* already loaded */
	return 0;

    if (rc = LoadIt(pm,TRUE)) {
	if (hFileIn != -1) {
	    _lclose(hFileIn);
	    hFileIn = -1;
	}
	if (pm->hMemCode) {
	    GlobalFree(pm->hMemCode);
	    pm->hMemCode = 0;
	}
    }
    return rc;
}

INT LoadDesc(PMACRO pm)
/* load description, return 0 or error */
{
    INT rc;

    if (pm->hMemDesc || pm->f.cchDesc == 0)
	/* already loaded, or no description at all */
	return 0;

    if (rc = LoadIt(pm,FALSE)) {
	if (hFileIn != -1) {
	    _lclose(hFileIn);
	    hFileIn = -1;
	}
	if (pm->hMemDesc) {
	    GlobalFree(pm->hMemDesc);
	    pm->hMemDesc = 0;
	}
    }
    return rc;
}

static INT NEAR LoadIt(PMACRO pm, BOOL bMacro)
/* load macro or desc from file. if error, caller must close hFileIn if not -1
   (and set to -1), free hMemCode/Desc if non-zero (and zero hMemCode/Desc)
*/
{
    WORD w;
    WORD rc,cch;
    HANDLE hMem;
    LPRECORDERMSG lpt;
    CHAR *szErrSave;

    szErrSave = szErrArg;   /* may be in SAVE operation - remember outfile */
    szErrArg = szFileName;

    /* reopen input file */
    if ((hFileIn = OpenFile(szFileName,&ofCurrent,OF_READ|OF_REOPEN)) == -1)
	/* return READ error - else get weird OPEN error with advise
	    to check spelling */
	return E_READ;

#ifdef NTPORT

    /* find start of block */
    if (_llseek(hFileIn,bMacro ? pm->f.lPosCode : pm->f.lPosDesc,0) < 0)
	return E_READ;

    /* check magic number */
    if (rc=_lread(hFileIn,(LPSTR)&w,sizeof(WORD))==-1) return E_READ;

#else

    /* find start of block */
    if (_lseek(hFileIn,bMacro ? pm->f.lPosCode : pm->f.lPosDesc,SEEK_SET) < 0)
	return E_READ;

    /* check magic number */
    if (_dos_read(hFileIn,&w,sizeof(WORD),&rc) != 0) return E_READ;

#endif

    if (w != (WORD)(bMacro ? MAGIC_CODE : MAGIC_DESC))
	return E_BADFILE;

    /* read data itself into fresh global memory block */
    cch = (bMacro ? pm->f.cchCode : pm->f.cchDesc);
    if ((hMem = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,(LONG) cch)) == 0)
	return E_NOMEM;

#ifdef NTPORT
    rc = _lread(hFileIn,GlobalLock(hMem),cch);
#else
    rc = lread(hFileIn,GlobalLock(hMem),cch);
#endif

    GlobalUnlock(hMem);
    if (bMacro) {
	/* set hMem, but also set flags word in first message,
	    may have changed due to edit and not saved to disk */
	lpt = (LPRECORDERMSG) GlobalLock(pm->hMemCode = hMem);
	lpt->message = pm->f.flags;
	GlobalUnlock(pm->hMemCode);
    }
    else
	pm->hMemDesc = hMem;
    if (rc != cch)
	return E_READ;
    _lclose(hFileIn);
    hFileIn = -1;
    szErrArg = szErrSave;
    return 0;
}

INT RecReadFile(CHAR *szFile,POFSTRUCT pof)
/* on error, caller must clear hot keys, close hFileIn if not -1 */
{
    struct header hdr;
    INT i;
    UINT rc;
    WORD w;
    PMACRO pm;

    if (OpenFile(szFile,pof,OF_PARSE) < 0)
	return E_BADFILENAME;
    if ((hFileIn = OpenFile(szFile,pof,OF_READ)) == -1)
	return E_CANTOPEN;

#ifdef NTPORT
    if ((rc=_lread(hFileIn,(LPSTR)&hdr,sizeof(struct header)))==-1 ||
	rc != sizeof(struct header)) return E_READ;
#else
    if (_dos_read(hFileIn,&hdr,sizeof(struct header),&rc) != 0
      || rc != sizeof(struct header))
	return E_READ;
#endif

    if (hdr.magic != MAGIC_HDR)
	return E_NOTRECORDERFILE;
    if (hdr.version != FVERSION)
	return E_BADVERSION;

#ifdef NTPORT
    if (_llseek(hFileIn,hdr.lPosMacros,0)==-1)
	return E_READ;

    if ((rc=_lread(hFileIn,(LPSTR)&w,sizeof(WORD)))==-1 || rc != sizeof(WORD))
	return E_READ;
#else
    if (_lseek(hFileIn,hdr.lPosMacros,SEEK_SET) == -1)
	return E_READ;

    if (_dos_read(hFileIn,&w,sizeof(WORD),&rc) != 0 || rc != sizeof(WORD))
	return E_READ;
#endif

    if (w != MAGIC_MACRO)
	return E_BADFILE;
    for (i = hdr.cMacros; i > 0; i--) {
	if ((pm = AllocLocal(sizeof(struct macro))) == 0)
	    return E_NOMEM;

#ifdef NTPORT
	if ((rc=_lread(hFileIn,(LPSTR)&pm->f,sizeof(struct filemacro)))==-1 ||
	     rc != sizeof(struct filemacro)) {
	    FreeLocal(pm);
	    return E_READ;
	}
#else
	if (_dos_read(hFileIn,(CHAR *)&pm->f,sizeof(struct filemacro),&rc)
	  != 0 || rc != sizeof(struct filemacro)) {
	    FreeLocal(pm);
	    return E_READ;
	}
#endif

	/* add to macro list */
	AddMacro(pm);
    }
    _lclose(hFileIn);
    hFileIn = -1;
    return 0;
}
/* answers: Spanish discovery of America (although CC was 12,000 miles
   from where he thought he was - the most lost explorer in history).
   US Declaration of Independence, US Constitution, landing on moon */
/* ------------------------------ EOF --------------------------------- */
