/**************************************************************************
NAME

    util.c -- misc utilities for Recorder

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
    sds		30 Nov 88	Initial Coding
		9 Dec		added AddMacro
		15 Dec		CheckHotKey flags now a word
		19 Dec		now using PMACRO typedef
		22 Dec		added ListKeyCodes
		23 Dec		upgrades for Win30
		3 Jan 89	upgrade for M model
		5 Jan		ErrMsg now loads M_ERROR
				Improved error msg handling
		6 Jan		made internal funcs static
		2 Feb 89	FreeMacro now frees font face
		1 March 89	FreeMacro no longer frees font face
		13 Mar 89	restored Win2.1 support
		16 Mar 89	added ErrMsgArg
		29 Mar 1989	prettier errormsg for cmd line stuff
		30 Mar 1989	preloaded M_ERROR,M_WARNING
		2 May 1989	now checking LockResource ret code
		9 May 1989	just removed some unused #ifdef stuff
		16 May 1989	tdyn.h now tracerd.h
		18 May 1989	massive user intfc changes
    sds		23 May 1989	no longer releases code/desc if file changed
    sds		9 Jun 1989	name change to recorder
				fixed bug in GetKeyCode string matching
				ena/disable LB to prevent focus if empty
    sds		16 Jun 1989	now turns off redraw when loading listbox
    sds		3 Jul 1989	now using wsprintf
    sds		27 Jul 1989	AnsiUpper, not toupper
				ifdef'ed out local heap debugging
				totally new keycode/name resource handling
    sds		28 Jul 1989	Yet another keycode/name resource handler,
				since Win2 and Win3 RC have different
				sets of problems generating '\0' data bytes
    sds		17 Aug 1989	AddMacro now sets playback id
    sds		10 Oct 1989	LoadMacroList now enables listbox
    sds		31 Oct 1989	FreeMacro now takes bDelHotKey arg
    sds		26 Dec 1989	CheckComment
    sds		8 Feb 1990	now set focus to hWndRecorder if LB empty
    				Win2.1 compatibility
				now disallows unmappable keys as shortcuts
    sds 	7 Jan 1991	cleanups for MSC6 W4
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
#ifdef WIN2
#include "controls.h"
#endif
#include "recordll.h"
#include "recorder.h"
#include "msgs.h"

/* PREPROCESSOR DEFINITIONS */

/* FUNCTIONS DEFINED */
static VOID NEAR DoMsg(HWND hWnd,WORD ecode,CHAR *szCaption);
static BOOL NEAR LoadKbdStrings(VOID);

#ifdef HEAPDEBUG
VOID NEAR HeapError(CHAR *psz);
#endif

/* VARIABLES DEFINED */
static HANDLE hrdKeys=0;
static CHAR szPlus[]="+";
static BYTE pbID=0;		    /* playback id */

/* FUNCTIONS REFERENCED */


/* VARIABLES REFERENCED */

/* -------------------------------------------------------------------- */
VOID AddMacro(PMACRO pmN)
{
    register PMACRO pm;

    /* set id used to identify playback */
    pmN->id = ++pbID;
    if (pmFirst) {
	for (pm = pmFirst; pm->pmNext; pm = pm->pmNext)
	    ;
	pm->pmNext = pmN;
    }
    else
	pmFirst = pmN;
    EnableWindow(hWndList,TRUE);
}

VOID FreeMacroList()
{
    register PMACRO pm;

    while (pm = pmFirst) {
	pmFirst = pm->pmNext;
	FreeMacro(pm,TRUE);
    }
    pmFirst = 0;
    /* make sure listbox empty too */
    LoadMacroList();
    if (GetFocus() == hWndList)
	/* disabling focus window is unhealthy - set to main window */
	SetFocus(hWndRecorder);
    EnableWindow(hWndList,FALSE);
}

VOID FreeMacro(PMACRO pm,BOOL bDelHotKey)
{
    if (pm->hMemCode)
	GlobalFree(pm->hMemCode);
    if (pm->hMemDesc)
	GlobalFree(pm->hMemDesc);
    if (bDelHotKey && pm->f.hotkey)
	DelHotKey(pm->f.hotkey,pm->f.flags);
    FreeLocal(pm);
}

VOID ReleaseDesc(PMACRO pm)
/* don't need description in memory any more */
{
    if (!bFileChanged && pm->hMemDesc && pm->f.lPosDesc != 0) {
	/* ok to free it, can reload if necessary */
	GlobalFree(pm->hMemDesc);
	pm->hMemDesc = 0;
    }
}

VOID ReleaseCode(PMACRO pm)
/* don't need code in memory any more */
{
    if (!bFileChanged && pm->hMemCode && pm->f.lPosCode != 0) {
	/* ok to free it, can reload if necessary */
	GlobalFree(pm->hMemCode);
	pm->hMemCode = 0;
    }
}

VOID LoadMacroList()
{
    register PMACRO pm;

    SendMessage(hWndList,WM_SETREDRAW,0,0L);
    SendMessage(hWndList,LB_RESETCONTENT,0,0L);

    for (pm = pmFirst; pm; pm = pm->pmNext) {
	HotKeyString(pm,TRUE);
	SendMessage(hWndList,LB_ADDSTRING,0,(LONG) (LPSTR) szScratch);
    }
    if (pmFirst) {
	EnableWindow(hWndList,TRUE);
	SendMessage(hWndList,LB_SETCURSEL,0,0L);
    }
    else if (GetFocus() == hWndList)
	/* get cursor out of empty listbox */
	SetFocus(hWndRecorder);
    SendMessage(hWndList,WM_SETREDRAW,1,0L);
    InvalidateRect(hWndList,(LPRECT)NULL,TRUE);
}

VOID HotKeyString(PMACRO pm,BOOL bWithComment)
/* build hot key description string in szScratch */
{
    BOOL b;
    CHAR szTemp[CCHSCRATCH];

    szScratch[0] = '\0';
    if (pm->f.hotkey) {
	b = FALSE;
	if (pm->f.flags & B_CONTROL) {
	    strcpy(szScratch,szControl);
	    b = TRUE;
	}
	if (pm->f.flags & B_SHIFT) {
	    if (b)
		strcat(szScratch,szPlus);
	    strcat(szScratch,szShift);
	    b = TRUE;
	}
	if (pm->f.flags & B_ALT) {
	    if (b)
		strcat(szScratch,szPlus);
	    strcat(szScratch,szAlt);
	    b = TRUE;
	}
	GetKeyString(pm->f.hotkey,szTemp);
	if (b)
	    strcat(szScratch,szPlus);
	strcat(szScratch,szTemp);
    }
    if (bWithComment && pm->f.szComment[0])
#ifdef WIN2
	sprintf(&szScratch[strlen(szScratch)],"  \"%s\"",pm->f.szComment);
#else
	wsprintf(&szScratch[strlen(szScratch)],"\t%s",(LPSTR)pm->f.szComment);
#endif
}

BOOL CheckHotKey(BYTE key,WORD flags,PMACRO pmEdit)
/* return FALSE if hot key already defined (except pmEdit) */
{
    register PMACRO pm;

    if (key)
	for (pm = pmFirst; pm; pm = pm->pmNext)
	    if (pm != pmEdit && pm->f.hotkey == key &&
	      (pm->f.flags & (B_SHFT_CONT_ALT)) ==
	      (flags & (B_SHFT_CONT_ALT)))
		return FALSE;
    return TRUE;
}

BOOL CheckComment(PMACRO pmEdit,CHAR *szComment)
/* return FALSE if comment is not unique.  Ignore hotkey in pmEdit,
   may not be valid */
{
    register PMACRO pm;

    if (*szComment)
	for (pm = pmFirst; pm; pm = pm->pmNext)
	    if (pm != pmEdit && pm->f.hotkey == 0 &&
	      _strcmpi(pm->f.szComment,szComment) == 0)
		return FALSE;
    return TRUE;
}

static BOOL NEAR LoadKbdStrings()
{
    if (hrdKeys == 0)
      hrdKeys = LoadResource( hInstRecorder,
        FindResource( hInstRecorder, MAKEINTRESOURCE(RKEYS), RT_RCDATA));
    return (hrdKeys != 0);
}

VOID GetKeyString(UCHAR key,CHAR *psz)
/* put string representation of key in szScratch */
{
    LPSTR lpsz, lpszK;

    /* assume is just the char itself */
    if (key >= ' ') {
	switch(key) {
	case ' ':
	case ',':
	    *psz = '\'';
	    *(psz + 1) = key;
	    *(psz + 2) = '\'';
	    *(psz + 3) = '\0';
	    break;
	default:
	    *psz = key;
	    *(psz + 1) = '\0';
	}
    }
    else
#ifdef WIN2
	sprintf(psz,"0x%x",key);
#else
	wsprintf(psz,"0x%x",key);
#endif
    if (!LoadKbdStrings())
	return;
    if ((lpsz = LockResource(hrdKeys)) == NULL)
	return;
    do {
	INT cch;

	lpsz += (cch = lstrlen(lpszK = lpsz) - 1);
	if (*lpsz == (CHAR)key) {
	    lmemcpy(psz,lpszK,cch);
	    *(psz + cch) = '\0';
	    break;
	}
	lpsz += 2;
    } while (*lpsz);

    UnlockResource(hrdKeys);
}

UCHAR GetKeyCode(CHAR *psz)
/* return key code for string representation, or 0 on error */
{

#ifdef NTPORT
    CHAR sz[2];
    LPSTR lpsz=&sz[0];
    UCHAR rc;
    INT cch, cchK;

    lpsz[0]=psz[0];
    lpsz[1]=psz[1];

#else
    UCHAR cchK,rc;
    UCHAR cch;
#endif

    if (*(psz + 1) == '\0') {

	/* just one char is always what it is */

#ifdef NTPORT
	rc = (UCHAR)((DWORD)(AnsiUpper(lpsz)));
#else
	rc = (UCHAR) AnsiUpper((LPSTR) MAKELONG(*psz,0));
#endif

	/* Sorry, international folks, but there is little alternative
	   here.  We are really mapping from ANSI to the virtual key
	   code we need for the keyboard hook.  This only works for
	   numbers and unaccented letters.  Punctuation keys, if left
	   alone by AnsiUpper, not only do not work as shortcut keys,
	   but their ANSI codes returned here as if they were VK_ codes
	   sometimes cause bizarre symptoms.  For example, the character
	   '.' yeilds a VK_DELETE, which is how the shortcut key would
	   display. */

	if ((rc >= '0' && rc <= '9') || (rc >= 'A' && rc <= 'Z'))
	    return rc;
	else
	    /* cannot map to VK_ - disallow this shortcut key */
	    return '\0';
    }

    if (!LoadKbdStrings() || (lpsz = LockResource(hrdKeys)) == 0)
	return '\0';

    cch =  strlen(psz);
    rc = '\0';
    do {
	cchK = lstrlen(lpsz) - 1;
	if (lstrncmpi(lpsz,(LPSTR) psz,max(cch,cchK)) == 0) {
	    /* got it */
	    rc = *(lpsz + cchK);
	    break;
	}
	lpsz += cchK + 2;
    } while (*lpsz);

    UnlockResource(hrdKeys);
    return rc;
}

VOID ListKeyCodes(HWND hWnd)
/* stuff key strings into combo box at hWnd */
{
    LPSTR lpsz;
    INT cch;

    if (!LoadKbdStrings())
	return;

    if ((lpsz = LockResource(hrdKeys)) == NULL)
	return;
    do {
	cch = (UCHAR) lstrlen(lpsz) - 1;
	if (*(lpsz + cch) == VK_MENU)
	    /* no more eligible for hot keys */
	    break;
	lmemcpy(szScratch,lpsz,cch);
	szScratch[cch] = '\0';
	SendMessage(hWnd,CB_ADDSTRING,0,(LONG) (LPSTR) szScratch);
	lpsz += cch + 2;
    } while (*lpsz);

    UnlockResource(hrdKeys);
}

VOID *AllocLocal(WORD wSize)
{
#ifdef HEAPDEBUG
    register CHAR *p;

    if (p = (VOID *)LocalAlloc(LPTR, wSize + 2 + sizeof(INT))) {
	*(INT *)p = wSize;
	p += sizeof(INT);
	*p++ = 0x12;
	*(p + wSize) = 0x34;
    }
    return (VOID *)p;
#else
    return (VOID *)LocalAlloc(LPTR, wSize);
#endif
}

VOID FreeLocal(VOID *p)
{
#ifdef HEAPDEBUG
    INT cch;
    register CHAR *pch;

    if (pch = p) {
	if (*(pch - 1) == 0x12) {
	    cch = *(INT *)(pch - 3);
	    if (*(pch + cch) != 0x34) {
		HeapError("FreeLocal - wrote too far");
		return;
	    }
	}
	else {
	    HeapError("FreeLocal - bogus free");
	    return;
	}
	LocalFree((HANDLE)pch - 3);
    }
#else
    LocalFree((HANDLE) p);
#endif
}

#if 0
VOID *ReallocLocal(VOID *p,WORD newsize)
{
#ifdef HEAPDEBUG
    INT cch;
    register CHAR *pch;

    pch = p;
    if (*(pch - 1) == 0x12) {
	cch = *(INT *)(pch - 3);
	if (*(pch + cch) != 0x34) {
	    HeapError("ReallocLocal - wrote too far");
	    return 0;
	}
    }
    else {
	HeapError("ReallocLocal - bogus ptr");
	return 0;
    }
    if (pch = (CHAR *)LocalReAlloc( (HANDLE)(pch - 3), newsize + 4, LHND )) {
	*(INT *)pch = newsize;
	/* 0x12 is still there */
	pch += 3;
	*(pch + newsize) = 0x34;
	return pch;
    }
    return 0;
#else
    return (VOID *)LocalReAlloc((HANDLE) p, newsize, LHND);
#endif
}
#endif /* if 0 */

#ifdef HEAPDEBUG
VOID NEAR HeapError(CHAR *psz)
{
    MessageBox(hWndRecorder,psz,(LPSTR) NULL,MB_OK| MB_APPLMODAL);
}
#endif

VOID ErrMsg(HWND hWnd,WORD ecode)
{
    DoMsg(hWnd,ecode,szError);
}

VOID WarnMsg(HWND hWnd,WORD ecode)
{
    DoMsg(hWnd,ecode,szWarning);
}

static VOID NEAR DoMsg(HWND hWnd,WORD ecode,CHAR *szCaption)
{
    CHAR *sz;

    EnaHotKeys(FALSE);
    if (ecode == E_NOMEM)
	sz = szNoMem;
    else {
	LdStr(hWnd,ecode);
	sz = szScratch;
    }
    MessageBox(hWnd, sz,szCaption,MB_OK|MB_APPLMODAL);
    EnaHotKeys(bHotKeys);
}


#ifdef NTPORT
VOID ErrMsgArg(HWND hWnd,WORD ecode,CHAR *sz)
/* this guy is called a lot even when called isn't sure error msg takes
    an arg or not.  Makes life a LOT easier. */
{
    CHAR szTemp[80];
    CHAR szP[CCHSCRATCH];

    if (ecode < 3000) {
	 ErrMsg(hWnd,ecode);
	 }
    else {
	 EnaHotKeys(FALSE);
	 LdStr(hWnd,ecode);

#ifdef WIN2
	 sprintf(szP,szScratch,sz);
#else
	 wsprintf(szP,szScratch,(LPSTR) sz);
#endif

	 if (ecode == E_BADCMDLINE || ecode == E_BOGUSHOTKEY) {
	 /* add correct syntax to error msg */
	      LoadString(hInstRecorder,M_CMDSYNTAX,szScratch,CCHSCRATCH);
	      strcat(szP,"\n");
	      strcat(szP,szScratch);
	      }

	 LoadString(hInstRecorder, M_ERROR, szTemp, 79);
	 MessageBox(hWnd, szP,szTemp,MB_OK|MB_APPLMODAL);
	 EnaHotKeys(bHotKeys);
	 }
}
#else
VOID ErrMsgArg(HWND hWnd,WORD ecode,CHAR *sz)
/* this guy is called a lot even when called isn't sure error msg takes
    an arg or not.  Makes life a LOT easier. */
{
    CHAR szTemp[80];
    CHAR szP[CCHSCRATCH];

    if (ecode < 3000)
	/* error type doesn't take an argument */
	return ErrMsg(hWnd,ecode);

    EnaHotKeys(FALSE);
    LdStr(hWnd,ecode);

#ifdef WIN2
    sprintf(szP,szScratch,sz);
#else
    wsprintf(szP,szScratch,(LPSTR) sz);
#endif

    if (ecode == E_BADCMDLINE || ecode == E_BOGUSHOTKEY) {
	/* add correct syntax to error msg */
	LoadString(hInstRecorder,M_CMDSYNTAX,szScratch,CCHSCRATCH);
	strcat(szP,"\n");
	strcat(szP,szScratch);
    }
    LoadString(hInstRecorder, M_ERROR, szTemp, 79);
    MessageBox(hWnd, szP,szTemp,MB_OK|MB_APPLMODAL);
    EnaHotKeys(bHotKeys);
}
#endif

VOID RecorderMsg(WORD ecode)
{
    LdStr(hWndRecorder,ecode);
    MessageBox(hWndRecorder, szScratch, szCaption,MB_OK|MB_APPLMODAL);
}

BOOL HasWildCard(register CHAR *sz)
/* return TRUE if file spec has a wild card */
{
    while (*sz) {
	if (*sz == CHWILDFILE || *sz == CHWILDCHAR)
	    return TRUE;
	sz++;
    }
    return FALSE;
}
/* ------------------------------ EOF --------------------------------- */
